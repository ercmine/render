#pragma once

#include "engine/core/camera.hpp"
#include "engine/core/transform.hpp"
#include "engine/render/buffer_types.hpp"
#include "engine/render/renderer_types.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace render::scene {

constexpr std::uint32_t kVisibilityAll = 0xFFFFFFFFu;

struct SceneNodeId {
  std::uint32_t index{0xFFFFFFFFu};
  std::uint32_t generation{0};

  [[nodiscard]] bool is_valid() const noexcept { return index != 0xFFFFFFFFu; }
  friend bool operator==(const SceneNodeId&, const SceneNodeId&) = default;
};

enum class ReparentPolicy : std::uint8_t {
  KeepLocalTransform = 0,
  KeepWorldTransform,
};

enum class LightType : std::uint8_t {
  Directional = 0,
  Point,
};

enum class CameraProjection : std::uint8_t {
  Perspective = 0,
  Orthographic,
};

struct NodeVisibility {
  bool enabled{true};
  std::uint32_t layer_mask{kVisibilityAll};
};

struct CameraComponent {
  bool enabled{true};
  std::uint32_t layer_mask{kVisibilityAll};
  CameraProjection projection{CameraProjection::Perspective};
  core::PerspectiveCameraParams perspective{};
  core::OrthographicCameraParams orthographic{};
};

struct LightComponent {
  bool enabled{true};
  LightType type{LightType::Directional};
  core::Vec3 color{1.0F, 1.0F, 1.0F};
  float intensity{1.0F};
  float range{10.0F};
};

struct RenderableComponent {
  bool enabled{true};
  std::uint32_t layer_mask{kVisibilityAll};
  rendering::MeshBufferHandle mesh_buffer{};
  rendering::IndexBufferHandle index_buffer{};
  std::uint32_t index_count{0};
  rendering::MaterialBinding material{};
  rendering::DrawState draw_state{};
};

struct CameraView {
  SceneNodeId node{};
  core::Mat4 view{};
  core::Mat4 projection{};
};

struct VisibleRenderable {
  SceneNodeId node{};
  const RenderableComponent* renderable{nullptr};
  const core::Transform* world_transform{nullptr};
};

struct VisibleLight {
  SceneNodeId node{};
  const LightComponent* light{nullptr};
  const core::Transform* world_transform{nullptr};
};

class Scene {
public:
  [[nodiscard]] SceneNodeId create_node(std::string debug_name = {});
  bool destroy_node(SceneNodeId node);

  [[nodiscard]] bool is_alive(SceneNodeId node) const;

  bool set_parent(SceneNodeId node, SceneNodeId parent, ReparentPolicy policy = ReparentPolicy::KeepWorldTransform);
  bool clear_parent(SceneNodeId node, ReparentPolicy policy = ReparentPolicy::KeepWorldTransform);

  [[nodiscard]] std::optional<SceneNodeId> parent(SceneNodeId node) const;
  [[nodiscard]] const std::vector<SceneNodeId>* children(SceneNodeId node) const;
  [[nodiscard]] std::vector<SceneNodeId> root_nodes() const;

  bool set_local_transform(SceneNodeId node, const core::Transform& local);
  [[nodiscard]] const core::Transform* local_transform(SceneNodeId node) const;
  [[nodiscard]] const core::Transform* world_transform(SceneNodeId node) const;

  bool set_visibility(SceneNodeId node, NodeVisibility visibility);
  [[nodiscard]] std::optional<NodeVisibility> visibility(SceneNodeId node) const;

  bool set_debug_name(SceneNodeId node, std::string name);
  [[nodiscard]] const std::string* debug_name(SceneNodeId node) const;
  [[nodiscard]] std::vector<SceneNodeId> find_nodes_by_name(std::string_view name) const;

  bool set_camera(SceneNodeId node, const CameraComponent& camera);
  bool clear_camera(SceneNodeId node);
  [[nodiscard]] const CameraComponent* camera(SceneNodeId node) const;

  bool set_light(SceneNodeId node, const LightComponent& light);
  bool clear_light(SceneNodeId node);
  [[nodiscard]] const LightComponent* light(SceneNodeId node) const;

  bool set_renderable(SceneNodeId node, const RenderableComponent& renderable);
  bool clear_renderable(SceneNodeId node);
  [[nodiscard]] const RenderableComponent* renderable(SceneNodeId node) const;

  bool set_active_camera(SceneNodeId node);
  void clear_active_camera();
  [[nodiscard]] std::optional<SceneNodeId> active_camera() const { return active_camera_; }

  void update_world_transforms();

  [[nodiscard]] std::optional<CameraView> build_camera_view(float aspect_ratio) const;
  [[nodiscard]] std::vector<VisibleRenderable> collect_visible_renderables(std::uint32_t layer_mask = kVisibilityAll) const;
  [[nodiscard]] std::vector<VisibleLight> collect_visible_lights(std::uint32_t layer_mask = kVisibilityAll) const;

  void walk_depth_first(const std::function<void(SceneNodeId)>& visitor) const;

  [[nodiscard]] std::size_t node_count() const noexcept;

private:
  struct NodeRecord {
    bool alive{false};
    std::uint32_t generation{0};
    std::optional<SceneNodeId> parent{};
    std::vector<SceneNodeId> children{};
    core::Transform local{};
    core::Transform world{};
    bool world_dirty{true};
    NodeVisibility visibility{};
    std::string debug_name{};
    std::optional<CameraComponent> camera{};
    std::optional<LightComponent> light{};
    std::optional<RenderableComponent> renderable{};
  };

  [[nodiscard]] NodeRecord* lookup(SceneNodeId node);
  [[nodiscard]] const NodeRecord* lookup(SceneNodeId node) const;
  [[nodiscard]] bool would_create_cycle(SceneNodeId node, SceneNodeId parent) const;
  void mark_world_dirty_recursive(SceneNodeId node);
  void detach_from_parent(SceneNodeId node);
  void destroy_subtree(SceneNodeId node);

  static core::Transform relative_transform(const core::Transform& parent_world, const core::Transform& world);

  std::vector<NodeRecord> nodes_{};
  std::vector<std::uint32_t> free_indices_{};
  std::optional<SceneNodeId> active_camera_{};
};

}  // namespace render::scene
