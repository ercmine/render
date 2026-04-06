#include "engine/render/shader_library.hpp"

#include "engine/platform/platform_log.hpp"

#include <sstream>

namespace render::rendering {

namespace {

std::string make_watch_key(const ShaderProgramId& id) {
  return id.category + ":" + id.name + ":" + id.variant;
}

}  // namespace

std::string backend_shader_folder(const RendererBackend backend) {
  switch (backend) {
    case RendererBackend::Direct3D11:
    case RendererBackend::Direct3D12: return "dx11";
    case RendererBackend::Metal: return "metal";
    case RendererBackend::Vulkan: return "spirv";
    case RendererBackend::OpenGL: return "glsl";
    case RendererBackend::Noop:
    case RendererBackend::Auto:
    default: return "spirv";
  }
}

ShaderProgramPaths resolve_shader_program_paths(
  const filesystem::FileSystemService& filesystem,
  const RendererBackend backend,
  const ShaderPipelineLayout& layout,
  const ShaderProgramId& id) {
  const std::filesystem::path backend_dir = backend_shader_folder(backend);
  const std::filesystem::path shader_rel = backend_dir / id.category / id.name / id.variant;

  ShaderProgramPaths paths{};
  paths.vertex_binary = filesystem.root(filesystem::PathCategory::InstallRoot) / layout.binaries_relative_root / shader_rel / "vs.bin";
  paths.fragment_binary = filesystem.root(filesystem::PathCategory::InstallRoot) / layout.binaries_relative_root / shader_rel / "fs.bin";
  paths.program_metadata = filesystem.root(filesystem::PathCategory::InstallRoot) / layout.metadata_relative_root / shader_rel / "program.json";
  return paths;
}

ShaderProgramLibrary::ShaderProgramLibrary(
  Renderer& renderer,
  const filesystem::FileSystemService& filesystem,
  ShaderPipelineLayout layout)
  : renderer_(renderer),
    filesystem_(filesystem),
    layout_(std::move(layout)) {}

ProgramHandle ShaderProgramLibrary::load_program(const ShaderProgramId& id) {
  const ShaderProgramPaths paths = resolve_shader_program_paths(filesystem_, renderer_.backend(), layout_, id);

  if (!filesystem_.is_regular_file(paths.vertex_binary) || !filesystem_.is_regular_file(paths.fragment_binary)) {
    std::ostringstream error;
    error << "Shader program binaries missing for " << id.category << "/" << id.name << " variant=" << id.variant
          << " vs=" << paths.vertex_binary.string() << " fs=" << paths.fragment_binary.string();
    platform::log::error(error.str());
    return {};
  }

  if (!filesystem_.is_regular_file(paths.program_metadata)) {
    platform::log::warn(std::string{"Shader metadata missing: "} + paths.program_metadata.string());
  }

  ShaderProgramDescription description{};
  description.vertex_shader_path = paths.vertex_binary;
  description.fragment_shader_path = paths.fragment_binary;
  description.debug_name = id.category + "/" + id.name + "/" + id.variant;

  ProgramHandle handle = renderer_.create_program(description);
  if (handle.idx == kInvalidHandle) {
    std::ostringstream error;
    error << "Failed to create shader program for " << description.debug_name;
    platform::log::error(error.str());
    return {};
  }

  ProgramWatchState state{};
  if (collect_write_times(paths, state)) {
    watch_states_[make_watch_key(id)] = state;
  }

  return handle;
}

bool ShaderProgramLibrary::reload_if_stale(const ShaderProgramId& id, ProgramHandle& in_out_handle) {
  const std::string key = make_watch_key(id);
  const ShaderProgramPaths paths = resolve_shader_program_paths(filesystem_, renderer_.backend(), layout_, id);

  ProgramWatchState current{};
  if (!collect_write_times(paths, current)) {
    return false;
  }

  auto it = watch_states_.find(key);
  if (it == watch_states_.end()) {
    watch_states_.emplace(key, current);
    return false;
  }

  if (!is_newer(it->second, current)) {
    return false;
  }

  ProgramHandle replacement = load_program(id);
  if (replacement.idx == kInvalidHandle) {
    platform::log::warn("Shader hot reload failed; continuing with previous valid program");
    return false;
  }

  if (in_out_handle.idx != kInvalidHandle) {
    renderer_.destroy_program(in_out_handle);
  }
  in_out_handle = replacement;
  watch_states_[key] = current;
  platform::log::info(std::string{"Hot reloaded shader program: "} + key);
  return true;
}

bool ShaderProgramLibrary::collect_write_times(const ShaderProgramPaths& paths, ProgramWatchState& out_state) const {
  if (!filesystem_.is_regular_file(paths.vertex_binary) || !filesystem_.is_regular_file(paths.fragment_binary)) {
    return false;
  }

  std::error_code ec;
  out_state.vertex_write_time = std::filesystem::last_write_time(paths.vertex_binary, ec);
  if (ec) {
    platform::log::warn(std::string{"Failed to stat shader file: "} + paths.vertex_binary.string());
    return false;
  }

  out_state.fragment_write_time = std::filesystem::last_write_time(paths.fragment_binary, ec);
  if (ec) {
    platform::log::warn(std::string{"Failed to stat shader file: "} + paths.fragment_binary.string());
    return false;
  }

  if (filesystem_.is_regular_file(paths.program_metadata)) {
    out_state.metadata_write_time = std::filesystem::last_write_time(paths.program_metadata, ec);
    if (ec) {
      platform::log::warn(std::string{"Failed to stat shader metadata file: "} + paths.program_metadata.string());
      return false;
    }
  }

  return true;
}

bool ShaderProgramLibrary::is_newer(const ProgramWatchState& a, const ProgramWatchState& b) {
  return b.vertex_write_time > a.vertex_write_time || b.fragment_write_time > a.fragment_write_time
      || b.metadata_write_time > a.metadata_write_time;
}

}  // namespace render::rendering
