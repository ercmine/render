#pragma once

#include "engine/core/types.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace render::core {

template <typename T>
constexpr T clamp(const T value, const T lo, const T hi) noexcept {
  return std::min(std::max(value, lo), hi);
}

template <typename T>
constexpr T lerp(const T a, const T b, const f32 t) noexcept {
  return a + (b - a) * t;
}

inline bool nearly_equal(const f32 a, const f32 b, const f32 eps = kEpsilon) noexcept {
  return std::fabs(a - b) <= eps;
}

struct Vec2 {
  f32 x{0.0F};
  f32 y{0.0F};
};

struct Vec3 {
  f32 x{0.0F};
  f32 y{0.0F};
  f32 z{0.0F};
};

struct Vec4 {
  f32 x{0.0F};
  f32 y{0.0F};
  f32 z{0.0F};
  f32 w{0.0F};
};

inline constexpr Vec3 operator+(const Vec3 a, const Vec3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline constexpr Vec3 operator-(const Vec3 a, const Vec3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline constexpr Vec3 operator*(const Vec3 v, const f32 s) noexcept { return {v.x * s, v.y * s, v.z * s}; }
inline constexpr Vec3 operator/(const Vec3 v, const f32 s) noexcept { return {v.x / s, v.y / s, v.z / s}; }

inline constexpr f32 dot(const Vec3 a, const Vec3 b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr Vec3 cross(const Vec3 a, const Vec3 b) noexcept {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline f32 length(const Vec3 v) noexcept { return std::sqrt(dot(v, v)); }
inline Vec3 normalize(const Vec3 v) noexcept {
  const f32 len = length(v);
  if (len <= kEpsilon) {
    return {};
  }
  return v / len;
}

struct Quaternion {
  f32 x{0.0F};
  f32 y{0.0F};
  f32 z{0.0F};
  f32 w{1.0F};

  static Quaternion identity() noexcept { return {}; }
  static Quaternion from_axis_angle(const Vec3 axis, const f32 angle_radians) noexcept;
};

Quaternion operator*(const Quaternion& a, const Quaternion& b) noexcept;

struct Mat4 {
  std::array<f32, 16> m{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F,
                       0.0F, 1.0F};

  static Mat4 identity() noexcept { return {}; }
  const f32* data() const noexcept { return m.data(); }

  static Mat4 translation(const Vec3& v) noexcept;
  static Mat4 scale(const Vec3& v) noexcept;
  static Mat4 rotation(const Quaternion& q) noexcept;
};

Mat4 operator*(const Mat4& a, const Mat4& b) noexcept;
Vec3 transform_point(const Mat4& m, const Vec3& p) noexcept;

}  // namespace render::core
