#pragma once

#include "engine/core/math.hpp"

namespace render::core {

struct ColorRGBA8 {
  u8 r{0};
  u8 g{0};
  u8 b{0};
  u8 a{255};
};

struct ColorRGBA {
  f32 r{0.0F};
  f32 g{0.0F};
  f32 b{0.0F};
  f32 a{1.0F};

  static ColorRGBA white() noexcept { return {1.0F, 1.0F, 1.0F, 1.0F}; }
  static ColorRGBA black() noexcept { return {0.0F, 0.0F, 0.0F, 1.0F}; }
  static ColorRGBA red() noexcept { return {1.0F, 0.0F, 0.0F, 1.0F}; }
  static ColorRGBA green() noexcept { return {0.0F, 1.0F, 0.0F, 1.0F}; }
  static ColorRGBA blue() noexcept { return {0.0F, 0.0F, 1.0F, 1.0F}; }
};

ColorRGBA8 to_rgba8(const ColorRGBA& color) noexcept;
ColorRGBA from_rgba8(const ColorRGBA8& color) noexcept;
ColorRGBA lerp(const ColorRGBA& a, const ColorRGBA& b, f32 t) noexcept;

f32 srgb_to_linear(f32 value) noexcept;
f32 linear_to_srgb(f32 value) noexcept;

}  // namespace render::core
