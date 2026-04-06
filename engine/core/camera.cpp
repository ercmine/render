#include "engine/core/camera.hpp"

#include <cmath>

namespace render::core {

Mat4 make_perspective(const PerspectiveCameraParams& params) noexcept {
  Mat4 out{};
  out.m.fill(0.0F);

  const f32 f = 1.0F / std::tan(params.vertical_fov_radians * 0.5F);
  out.m[0] = f / params.aspect_ratio;
  out.m[5] = f;
  out.m[10] = (params.far_plane + params.near_plane) / (params.near_plane - params.far_plane);
  out.m[11] = -1.0F;
  out.m[14] = (2.0F * params.far_plane * params.near_plane) / (params.near_plane - params.far_plane);
  return out;
}

Mat4 make_orthographic(const OrthographicCameraParams& params) noexcept {
  Mat4 out{};
  out.m.fill(0.0F);

  out.m[0] = 2.0F / (params.right - params.left);
  out.m[5] = 2.0F / (params.top - params.bottom);
  out.m[10] = -2.0F / (params.far_plane - params.near_plane);
  out.m[12] = -(params.right + params.left) / (params.right - params.left);
  out.m[13] = -(params.top + params.bottom) / (params.top - params.bottom);
  out.m[14] = -(params.far_plane + params.near_plane) / (params.far_plane - params.near_plane);
  out.m[15] = 1.0F;
  return out;
}

Mat4 make_look_at(const Vec3& eye, const Vec3& target, const Vec3& up) noexcept {
  const Vec3 forward = normalize(target - eye);
  const Vec3 right = normalize(cross(forward, up));
  const Vec3 corrected_up = cross(right, forward);

  Mat4 out{};
  out.m = {
      right.x, corrected_up.x, -forward.x, 0.0F,
      right.y, corrected_up.y, -forward.y, 0.0F,
      right.z, corrected_up.z, -forward.z, 0.0F,
      -dot(right, eye), -dot(corrected_up, eye), dot(forward, eye), 1.0F,
  };
  return out;
}

}  // namespace render::core
