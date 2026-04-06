#pragma once

#include "engine/filesystem/filesystem.hpp"
#include "engine/render/renderer.hpp"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace render::rendering {

struct ShaderProgramId {
  std::string category{};
  std::string name{};
  std::string variant{"default"};
};

struct ShaderPipelineLayout {
  std::filesystem::path binaries_relative_root{"shaders/bin"};
  std::filesystem::path metadata_relative_root{"shaders/metadata"};
};

struct ShaderProgramPaths {
  std::filesystem::path vertex_binary{};
  std::filesystem::path fragment_binary{};
  std::filesystem::path program_metadata{};
};

[[nodiscard]] std::string backend_shader_folder(RendererBackend backend);
[[nodiscard]] ShaderProgramPaths resolve_shader_program_paths(
  const filesystem::FileSystemService& filesystem,
  RendererBackend backend,
  const ShaderPipelineLayout& layout,
  const ShaderProgramId& id);

class ShaderProgramLibrary {
public:
  ShaderProgramLibrary(Renderer& renderer, const filesystem::FileSystemService& filesystem, ShaderPipelineLayout layout = {});

  [[nodiscard]] ProgramHandle load_program(const ShaderProgramId& id);
  [[nodiscard]] bool reload_if_stale(const ShaderProgramId& id, ProgramHandle& in_out_handle);

private:
  struct ProgramWatchState {
    std::filesystem::file_time_type vertex_write_time{};
    std::filesystem::file_time_type fragment_write_time{};
    std::filesystem::file_time_type metadata_write_time{};
  };

  [[nodiscard]] bool collect_write_times(const ShaderProgramPaths& paths, ProgramWatchState& out_state) const;
  [[nodiscard]] bool is_newer(const ProgramWatchState& a, const ProgramWatchState& b);

  Renderer& renderer_;
  const filesystem::FileSystemService& filesystem_;
  ShaderPipelineLayout layout_;
  std::unordered_map<std::string, ProgramWatchState> watch_states_;
};

}  // namespace render::rendering
