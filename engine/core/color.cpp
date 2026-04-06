#include "engine/core/color.hpp"

#include <cmath>

namespace render::core {

ColorRGBA8 to_rgba8(const ColorRGBA& color) noexcept {
  return {
      static_cast<u8>(clamp(color.r, 0.0F, 1.0F) * 255.0F + 0.5F),
      static_cast<u8>(clamp(color.g, 0.0F, 1.0F) * 255.0F + 0.5F),
      static_cast<u8>(clamp(color.b, 0.0F, 1.0F) * 255.0F + 0.5F),
      static_cast<u8>(clamp(color.a, 0.0F, 1.0F) * 255.0F + 0.5F),
  };
}

ColorRGBA from_rgba8(const ColorRGBA8& color) noexcept {
  return {
      static_cast<f32>(color.r) / 255.0F,
      static_cast<f32>(color.g) / 255.0F,
      static_cast<f32>(color.b) / 255.0F,
      static_cast<f32>(color.a) / 255.0F,
  };
}

ColorRGBA lerp(const ColorRGBA& a, const ColorRGBA& b, const f32 t) noexcept {
  return {
      render::core::lerp(a.r, b.r, t),
      render::core::lerp(a.g, b.g, t),
      render::core::lerp(a.b, b.b, t),
      render::core::lerp(a.a, b.a, t),
  };
}

f32 srgb_to_linear(const f32 value) noexcept {
  if (value <= 0.04045F) {
    return value / 12.92F;
  }
  return std::pow((value + 0.055F) / 1.055F, 2.4F);
}

f32 linear_to_srgb(const f32 value) noexcept {
  if (value <= 0.0031308F) {
    return value * 12.92F;
  }
  return 1.055F * std::pow(value, 1.0F / 2.4F) - 0.055F;
}

}  // namespace render::core
