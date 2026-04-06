#pragma once

#include "engine/render/renderer_types.hpp"

#include <cstdint>

namespace render::rendering {

struct TextureDescription {
  std::uint16_t width{1};
  std::uint16_t height{1};
  bool has_mips{false};
  bool is_srgb{false};
};

struct SolidColorTextureDescription {
  TextureDescription texture{};
  std::uint8_t r{255};
  std::uint8_t g{255};
  std::uint8_t b{255};
  std::uint8_t a{255};
};

}  // namespace render::rendering
