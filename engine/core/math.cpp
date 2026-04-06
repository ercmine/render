#include "engine/core/math.hpp"

namespace render::core {

Quaternion Quaternion::from_axis_angle(const Vec3 axis, const f32 angle_radians) noexcept {
  const Vec3 naxis = normalize(axis);
  const f32 half = angle_radians * 0.5F;
  const f32 s = std::sin(half);
  return {naxis.x * s, naxis.y * s, naxis.z * s, std::cos(half)};
}

Quaternion operator*(const Quaternion& a, const Quaternion& b) noexcept {
  return {
      a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
      a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
  };
}

Mat4 Mat4::translation(const Vec3& v) noexcept {
  Mat4 out{};
  out.m[12] = v.x;
  out.m[13] = v.y;
  out.m[14] = v.z;
  return out;
}

Mat4 Mat4::scale(const Vec3& v) noexcept {
  Mat4 out{};
  out.m[0] = v.x;
  out.m[5] = v.y;
  out.m[10] = v.z;
  return out;
}

Mat4 Mat4::rotation(const Quaternion& q) noexcept {
  const f32 xx = q.x * q.x;
  const f32 yy = q.y * q.y;
  const f32 zz = q.z * q.z;
  const f32 xy = q.x * q.y;
  const f32 xz = q.x * q.z;
  const f32 yz = q.y * q.z;
  const f32 wx = q.w * q.x;
  const f32 wy = q.w * q.y;
  const f32 wz = q.w * q.z;

  Mat4 out{};
  out.m = {
      1.0F - 2.0F * (yy + zz), 2.0F * (xy + wz),        2.0F * (xz - wy),        0.0F,
      2.0F * (xy - wz),        1.0F - 2.0F * (xx + zz), 2.0F * (yz + wx),        0.0F,
      2.0F * (xz + wy),        2.0F * (yz - wx),        1.0F - 2.0F * (xx + yy), 0.0F,
      0.0F,                    0.0F,                    0.0F,                    1.0F,
  };
  return out;
}

Mat4 operator*(const Mat4& a, const Mat4& b) noexcept {
  Mat4 out{};
  out.m.fill(0.0F);
  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      for (int k = 0; k < 4; ++k) {
        out.m[c * 4 + r] += a.m[k * 4 + r] * b.m[c * 4 + k];
      }
    }
  }
  return out;
}

Vec3 transform_point(const Mat4& m, const Vec3& p) noexcept {
  return {
      m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12],
      m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
      m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14],
  };
}

}  // namespace render::core
