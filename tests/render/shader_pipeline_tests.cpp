#include "engine/filesystem/filesystem.hpp"
#include "engine/render/shader_library.hpp"

#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
  using namespace render;

  const fs::path sandbox = fs::temp_directory_path() / "render_shader_pipeline_tests";
  std::error_code ec;
  fs::remove_all(sandbox, ec);

  filesystem::StorageConfig config{};
  config.executable_path = sandbox / "bin" / "render_shell";
  config.source_root_override = sandbox / "source_data";
  config.writable_root_override = sandbox / "user";
  fs::create_directories(*config.source_root_override, ec);

  filesystem::FileSystemService file_system;
  assert(file_system.initialize(config));

  rendering::ShaderPipelineLayout layout{};
  rendering::ShaderProgramId id{
    .category = "debug",
    .name = "debug_triangle",
    .variant = "default",
  };

  const rendering::ShaderProgramPaths paths =
    rendering::resolve_shader_program_paths(file_system, rendering::RendererBackend::Vulkan, layout, id);

  const fs::path expected_base = file_system.root(filesystem::PathCategory::InstallRoot) / "shaders";
  assert(paths.vertex_binary == expected_base / "bin" / "spirv" / "debug" / "debug_triangle" / "default" / "vs.bin");
  assert(paths.fragment_binary == expected_base / "bin" / "spirv" / "debug" / "debug_triangle" / "default" / "fs.bin");
  assert(paths.program_metadata == expected_base / "metadata" / "spirv" / "debug" / "debug_triangle" / "default" / "program.json");

  assert(rendering::backend_shader_folder(rendering::RendererBackend::Direct3D11) == "dx11");
  assert(rendering::backend_shader_folder(rendering::RendererBackend::OpenGL) == "glsl");
  assert(rendering::backend_shader_folder(rendering::RendererBackend::Metal) == "metal");

  fs::remove_all(sandbox, ec);
  return 0;
}
