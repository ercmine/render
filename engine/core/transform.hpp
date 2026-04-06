#pragma once

#include "engine/core/math.hpp"

namespace render::core {

struct Transform {
  Vec3 translation{0.0F, 0.0F, 0.0F};
  Quaternion rotation{};
  Vec3 scale{1.0F, 1.0F, 1.0F};

  [[nodiscard]] Mat4 to_matrix() const noexcept;
  static Transform identity() noexcept { return {}; }
};

Transform compose(const Transform& parent, const Transform& local) noexcept;

Vec3 forward(const Transform& transform) noexcept;
Vec3 right(const Transform& transform) noexcept;
Vec3 up(const Transform& transform) noexcept;

}  // namespace render::core
