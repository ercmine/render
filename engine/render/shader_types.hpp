#pragma once

#include "engine/render/renderer_types.hpp"

#include <filesystem>
#include <string>

namespace render::rendering {

struct ShaderProgramDescription {
  std::filesystem::path vertex_shader_path{};
  std::filesystem::path fragment_shader_path{};
  std::string debug_name{};
};

}  // namespace render::rendering
