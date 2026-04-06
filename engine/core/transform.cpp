#include "engine/core/transform.hpp"

namespace render::core {

Mat4 Transform::to_matrix() const noexcept {
  return Mat4::translation(translation) * Mat4::rotation(rotation) * Mat4::scale(scale);
}

Transform compose(const Transform& parent, const Transform& local) noexcept {
  Transform out{};
  out.rotation = parent.rotation * local.rotation;
  out.scale = {parent.scale.x * local.scale.x, parent.scale.y * local.scale.y, parent.scale.z * local.scale.z};
  out.translation = transform_point(parent.to_matrix(), local.translation);
  return out;
}

Vec3 forward(const Transform& transform) noexcept {
  return normalize(transform_point(Mat4::rotation(transform.rotation), {0.0F, 0.0F, -1.0F}));
}

Vec3 right(const Transform& transform) noexcept {
  return normalize(transform_point(Mat4::rotation(transform.rotation), {1.0F, 0.0F, 0.0F}));
}

Vec3 up(const Transform& transform) noexcept {
  return normalize(transform_point(Mat4::rotation(transform.rotation), {0.0F, 1.0F, 0.0F}));
}

}  // namespace render::core
