#pragma once

#include "engine/core/math.hpp"

namespace render::core {

struct PerspectiveCameraParams {
  f32 vertical_fov_radians{degrees_to_radians(60.0F)};
  f32 aspect_ratio{16.0F / 9.0F};
  f32 near_plane{0.1F};
  f32 far_plane{1000.0F};
};

struct OrthographicCameraParams {
  f32 left{-1.0F};
  f32 right{1.0F};
  f32 bottom{-1.0F};
  f32 top{1.0F};
  f32 near_plane{0.1F};
  f32 far_plane{1000.0F};
};

Mat4 make_perspective(const PerspectiveCameraParams& params) noexcept;
Mat4 make_orthographic(const OrthographicCameraParams& params) noexcept;
Mat4 make_look_at(const Vec3& eye, const Vec3& target, const Vec3& up) noexcept;

}  // namespace render::core
