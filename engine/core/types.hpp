#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace render::core {

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

inline constexpr f32 kPi = 3.14159265358979323846F;
inline constexpr f32 kTwoPi = 6.28318530717958647692F;
inline constexpr f32 kHalfPi = 1.57079632679489661923F;
inline constexpr f32 kEpsilon = 1e-6F;

inline constexpr std::size_t kInvalidIndex = std::numeric_limits<std::size_t>::max();

constexpr f32 degrees_to_radians(const f32 degrees) noexcept { return degrees * (kPi / 180.0F); }
constexpr f32 radians_to_degrees(const f32 radians) noexcept { return radians * (180.0F / kPi); }

}  // namespace render::core
