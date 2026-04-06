#include "engine/scene/scene.hpp"

#include "engine/core/assert.hpp"
#include "engine/core/log.hpp"

#include <algorithm>
#include <cmath>
#include <stack>

namespace render::scene {

namespace {

core::Vec3 multiply_components(const core::Vec3& a, const core::Vec3& b) {
  return {a.x * b.x, a.y * b.y, a.z * b.z};
}

core::Quaternion conjugate(const core::Quaternion& q) {
  return {-q.x, -q.y, -q.z, q.w};
}

core::Vec3 rotate_vector(const core::Quaternion& q, const core::Vec3& v) {
  const core::Quaternion p{v.x, v.y, v.z, 0.0F};
  const core::Quaternion rotated = q * p * conjugate(q);
  return {rotated.x, rotated.y, rotated.z};
}

core::Vec3 safe_divide_components(const core::Vec3& numer, const core::Vec3& denom) {
  const auto div = [](const float n, const float d) {
    return std::fabs(d) > core::kEpsilon ? (n / d) : 0.0F;
  };
  return {div(numer.x, denom.x), div(numer.y, denom.y), div(numer.z, denom.z)};
}

}  // namespace

SceneNodeId Scene::create_node(std::string debug_name) {
  std::uint32_t index = 0;
  if (!free_indices_.empty()) {
    index = free_indices_.back();
    free_indices_.pop_back();
  } else {
    index = static_cast<std::uint32_t>(nodes_.size());
    nodes_.push_back({});
  }

  NodeRecord& record = nodes_[index];
  record.alive = true;
  record.parent.reset();
  record.children.clear();
  record.local = core::Transform::identity();
  record.world = core::Transform::identity();
  record.world_dirty = true;
  record.visibility = {};
  record.debug_name = std::move(debug_name);
  record.camera.reset();
  record.light.reset();
  record.renderable.reset();
  record.vfx_attachment.reset();

  const SceneNodeId node{index, record.generation};
  RENDER_LOG_INFO("scene", "create node idx=", node.index, " name='", record.debug_name, "'");
  return node;
}

bool Scene::destroy_node(const SceneNodeId node) {
  if (!is_alive(node)) {
    return false;
  }

  if (active_camera_.has_value() && active_camera_.value() == node) {
    active_camera_.reset();
  }

  destroy_subtree(node);
  return true;
}

bool Scene::is_alive(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  return record != nullptr;
}

bool Scene::set_parent(const SceneNodeId node, const SceneNodeId parent, const ReparentPolicy policy) {
  if (!is_alive(node) || !is_alive(parent) || node == parent || would_create_cycle(node, parent)) {
    RENDER_LOG_WARN("scene", "rejected set_parent node=", node.index, " parent=", parent.index);
    return false;
  }

  NodeRecord* node_record = lookup(node);
  RENDER_ASSERT(node_record != nullptr, "node should be valid");

  const core::Transform prior_world = node_record->world;

  detach_from_parent(node);

  NodeRecord* parent_record = lookup(parent);
  RENDER_ASSERT(parent_record != nullptr, "parent should be valid");
  node_record->parent = parent;
  parent_record->children.push_back(node);

  if (policy == ReparentPolicy::KeepWorldTransform) {
    node_record->local = relative_transform(parent_record->world, prior_world);
  }

  mark_world_dirty_recursive(node);
  return true;
}

bool Scene::clear_parent(const SceneNodeId node, const ReparentPolicy policy) {
  if (!is_alive(node)) {
    return false;
  }

  NodeRecord* node_record = lookup(node);
  RENDER_ASSERT(node_record != nullptr, "node should be valid");
  if (!node_record->parent.has_value()) {
    return true;
  }

  const core::Transform prior_world = node_record->world;
  detach_from_parent(node);

  if (policy == ReparentPolicy::KeepWorldTransform) {
    node_record->local = prior_world;
  }

  mark_world_dirty_recursive(node);
  return true;
}

std::optional<SceneNodeId> Scene::parent(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return std::nullopt;
  }
  return record->parent;
}

const std::vector<SceneNodeId>* Scene::children(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return nullptr;
  }
  return &record->children;
}

std::vector<SceneNodeId> Scene::root_nodes() const {
  std::vector<SceneNodeId> roots;
  for (std::uint32_t i = 0; i < nodes_.size(); ++i) {
    const NodeRecord& record = nodes_[i];
    if (record.alive && !record.parent.has_value()) {
      roots.push_back({i, record.generation});
    }
  }
  return roots;
}

bool Scene::set_local_transform(const SceneNodeId node, const core::Transform& local) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->local = local;
  mark_world_dirty_recursive(node);
  return true;
}

const core::Transform* Scene::local_transform(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  return record == nullptr ? nullptr : &record->local;
}

const core::Transform* Scene::world_transform(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  return record == nullptr ? nullptr : &record->world;
}

bool Scene::set_visibility(const SceneNodeId node, const NodeVisibility visibility) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->visibility = visibility;
  return true;
}

std::optional<NodeVisibility> Scene::visibility(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return std::nullopt;
  }
  return record->visibility;
}

bool Scene::set_debug_name(const SceneNodeId node, std::string name) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->debug_name = std::move(name);
  return true;
}

const std::string* Scene::debug_name(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  return record == nullptr ? nullptr : &record->debug_name;
}

std::vector<SceneNodeId> Scene::find_nodes_by_name(const std::string_view name) const {
  std::vector<SceneNodeId> matches;
  for (std::uint32_t i = 0; i < nodes_.size(); ++i) {
    const NodeRecord& record = nodes_[i];
    if (record.alive && record.debug_name == name) {
      matches.push_back({i, record.generation});
    }
  }
  return matches;
}

bool Scene::set_camera(const SceneNodeId node, const CameraComponent& camera_component) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->camera = camera_component;
  return true;
}

bool Scene::clear_camera(const SceneNodeId node) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->camera.reset();
  if (active_camera_.has_value() && active_camera_.value() == node) {
    active_camera_.reset();
  }
  return true;
}

const CameraComponent* Scene::camera(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  if (record == nullptr || !record->camera.has_value()) {
    return nullptr;
  }
  return &record->camera.value();
}

bool Scene::set_light(const SceneNodeId node, const LightComponent& light_component) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->light = light_component;
  return true;
}

bool Scene::clear_light(const SceneNodeId node) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->light.reset();
  return true;
}

const LightComponent* Scene::light(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  if (record == nullptr || !record->light.has_value()) {
    return nullptr;
  }
  return &record->light.value();
}

bool Scene::set_renderable(const SceneNodeId node, const RenderableComponent& renderable_component) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->renderable = renderable_component;
  return true;
}

bool Scene::clear_renderable(const SceneNodeId node) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->renderable.reset();
  return true;
}

const RenderableComponent* Scene::renderable(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  if (record == nullptr || !record->renderable.has_value()) {
    return nullptr;
  }
  return &record->renderable.value();
}

bool Scene::set_vfx_attachment(const SceneNodeId node, const VfxAttachmentComponent& attachment_component) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->vfx_attachment = attachment_component;
  return true;
}

bool Scene::clear_vfx_attachment(const SceneNodeId node) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return false;
  }
  record->vfx_attachment.reset();
  return true;
}

const VfxAttachmentComponent* Scene::vfx_attachment(const SceneNodeId node) const {
  const NodeRecord* record = lookup(node);
  if (record == nullptr || !record->vfx_attachment.has_value()) {
    return nullptr;
  }
  return &record->vfx_attachment.value();
}

bool Scene::set_active_camera(const SceneNodeId node) {
  const NodeRecord* record = lookup(node);
  if (record == nullptr || !record->camera.has_value()) {
    return false;
  }
  active_camera_ = node;
  RENDER_LOG_INFO("scene", "active camera idx=", node.index);
  return true;
}

void Scene::clear_active_camera() {
  active_camera_.reset();
}

void Scene::update_world_transforms() {
  std::stack<SceneNodeId> stack;
  for (const SceneNodeId root : root_nodes()) {
    stack.push(root);
    while (!stack.empty()) {
      const SceneNodeId node = stack.top();
      stack.pop();

      NodeRecord* record = lookup(node);
      if (record == nullptr) {
        continue;
      }

      bool parent_dirty = false;
      if (record->parent.has_value()) {
        const NodeRecord* parent_record = lookup(record->parent.value());
        parent_dirty = parent_record != nullptr ? parent_record->world_dirty : false;
      }
      if (record->world_dirty || parent_dirty) {
        if (record->parent.has_value()) {
          const NodeRecord* parent_record = lookup(record->parent.value());
          record->world = core::compose(parent_record->world, record->local);
        } else {
          record->world = record->local;
        }
        record->world_dirty = false;
      }

      for (const SceneNodeId child : record->children) {
        stack.push(child);
      }
    }
  }
}

std::optional<CameraView> Scene::build_camera_view(const float aspect_ratio) const {
  std::optional<SceneNodeId> camera_node_id = active_camera_;
  if (!camera_node_id.has_value()) {
    for (std::uint32_t i = 0; i < nodes_.size(); ++i) {
      const NodeRecord& record = nodes_[i];
      if (record.alive && record.camera.has_value() && record.camera->enabled && record.visibility.enabled) {
        camera_node_id = SceneNodeId{i, record.generation};
        break;
      }
    }
  }

  if (!camera_node_id.has_value()) {
    return std::nullopt;
  }

  const NodeRecord* camera_node = lookup(camera_node_id.value());
  if (camera_node == nullptr || !camera_node->camera.has_value()) {
    return std::nullopt;
  }

  const CameraComponent& camera_component = camera_node->camera.value();
  if (!camera_component.enabled || !camera_node->visibility.enabled) {
    return std::nullopt;
  }

  core::Mat4 projection = core::Mat4::identity();
  if (camera_component.projection == CameraProjection::Perspective) {
    core::PerspectiveCameraParams params = camera_component.perspective;
    params.aspect_ratio = aspect_ratio;
    projection = core::make_perspective(params);
  } else {
    projection = core::make_orthographic(camera_component.orthographic);
  }

  const core::Vec3 eye = camera_node->world.translation;
  const core::Vec3 target = eye + core::forward(camera_node->world);
  const core::Vec3 up = core::up(camera_node->world);

  return CameraView{.node = camera_node_id.value(), .view = core::make_look_at(eye, target, up), .projection = projection};
}

std::vector<VisibleRenderable> Scene::collect_visible_renderables(const std::uint32_t layer_mask) const {
  std::vector<VisibleRenderable> out;
  out.reserve(nodes_.size());
  walk_depth_first([this, &out, layer_mask](const SceneNodeId node) {
    const NodeRecord* record = lookup(node);
    if (record == nullptr || !record->visibility.enabled || !record->renderable.has_value()) {
      return;
    }

    const RenderableComponent& renderable_component = record->renderable.value();
    if (!renderable_component.enabled) {
      return;
    }

    if ((record->visibility.layer_mask & layer_mask) == 0U) {
      return;
    }

    if ((renderable_component.layer_mask & layer_mask) == 0U) {
      return;
    }

    out.push_back({.node = node, .renderable = &renderable_component, .world_transform = &record->world});
  });
  return out;
}

std::vector<VisibleLight> Scene::collect_visible_lights(const std::uint32_t layer_mask) const {
  std::vector<VisibleLight> out;
  out.reserve(nodes_.size());
  walk_depth_first([this, &out, layer_mask](const SceneNodeId node) {
    const NodeRecord* record = lookup(node);
    if (record == nullptr || !record->visibility.enabled || !record->light.has_value()) {
      return;
    }

    if ((record->visibility.layer_mask & layer_mask) == 0U) {
      return;
    }

    const LightComponent& light_component = record->light.value();
    if (!light_component.enabled) {
      return;
    }

    out.push_back({.node = node, .light = &light_component, .world_transform = &record->world});
  });
  return out;
}

std::vector<VisibleRenderable> Scene::collect_highlighted_renderables(const std::uint32_t layer_mask) const {
  std::vector<VisibleRenderable> out;
  out.reserve(nodes_.size());
  walk_depth_first([this, &out, layer_mask](const SceneNodeId node) {
    const NodeRecord* record = lookup(node);
    if (record == nullptr || !record->visibility.enabled || !record->renderable.has_value()) {
      return;
    }

    const RenderableComponent& renderable_component = record->renderable.value();
    if (!renderable_component.enabled || !renderable_component.highlighted) {
      return;
    }

    if ((record->visibility.layer_mask & layer_mask) == 0U || (renderable_component.layer_mask & layer_mask) == 0U) {
      return;
    }

    out.push_back({.node = node, .renderable = &renderable_component, .world_transform = &record->world});
  });
  return out;
}

std::vector<VisibleVfxAttachment> Scene::collect_visible_vfx_attachments(const std::uint32_t layer_mask) const {
  std::vector<VisibleVfxAttachment> out;
  out.reserve(nodes_.size());
  walk_depth_first([this, &out, layer_mask](const SceneNodeId node) {
    const NodeRecord* record = lookup(node);
    if (record == nullptr || !record->visibility.enabled || !record->vfx_attachment.has_value()) {
      return;
    }

    const VfxAttachmentComponent& attachment_component = record->vfx_attachment.value();
    if (!attachment_component.enabled) {
      return;
    }

    if ((record->visibility.layer_mask & layer_mask) == 0U || (attachment_component.layer_mask & layer_mask) == 0U) {
      return;
    }

    out.push_back({.node = node, .attachment = &attachment_component, .world_transform = &record->world});
  });
  return out;
}

void Scene::set_lighting_settings(const SceneLightingSettings& settings) {
  lighting_settings_ = settings;
}

const SceneLightingSettings& Scene::lighting_settings() const noexcept {
  return lighting_settings_;
}

void Scene::walk_depth_first(const std::function<void(SceneNodeId)>& visitor) const {
  std::vector<SceneNodeId> stack = root_nodes();
  while (!stack.empty()) {
    const SceneNodeId node = stack.back();
    stack.pop_back();

    visitor(node);

    const NodeRecord* record = lookup(node);
    if (record == nullptr) {
      continue;
    }

    for (auto it = record->children.rbegin(); it != record->children.rend(); ++it) {
      stack.push_back(*it);
    }
  }
}

std::size_t Scene::node_count() const noexcept {
  std::size_t count = 0;
  for (const NodeRecord& record : nodes_) {
    if (record.alive) {
      ++count;
    }
  }
  return count;
}

Scene::NodeRecord* Scene::lookup(const SceneNodeId node) {
  if (node.index >= nodes_.size()) {
    return nullptr;
  }

  NodeRecord& record = nodes_[node.index];
  if (!record.alive || record.generation != node.generation) {
    return nullptr;
  }

  return &record;
}

const Scene::NodeRecord* Scene::lookup(const SceneNodeId node) const {
  if (node.index >= nodes_.size()) {
    return nullptr;
  }

  const NodeRecord& record = nodes_[node.index];
  if (!record.alive || record.generation != node.generation) {
    return nullptr;
  }

  return &record;
}

bool Scene::would_create_cycle(const SceneNodeId node, const SceneNodeId parent) const {
  std::optional<SceneNodeId> cursor = parent;
  while (cursor.has_value()) {
    if (cursor.value() == node) {
      return true;
    }

    const NodeRecord* record = lookup(cursor.value());
    if (record == nullptr) {
      return false;
    }
    cursor = record->parent;
  }

  return false;
}

void Scene::mark_world_dirty_recursive(const SceneNodeId node) {
  NodeRecord* record = lookup(node);
  if (record == nullptr) {
    return;
  }

  std::vector<SceneNodeId> stack;
  stack.push_back(node);
  while (!stack.empty()) {
    const SceneNodeId current = stack.back();
    stack.pop_back();

    NodeRecord* current_record = lookup(current);
    if (current_record == nullptr) {
      continue;
    }

    current_record->world_dirty = true;
    for (const SceneNodeId child : current_record->children) {
      stack.push_back(child);
    }
  }
}

void Scene::detach_from_parent(const SceneNodeId node) {
  NodeRecord* record = lookup(node);
  if (record == nullptr || !record->parent.has_value()) {
    return;
  }

  NodeRecord* parent_record = lookup(record->parent.value());
  if (parent_record != nullptr) {
    auto& siblings = parent_record->children;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), node), siblings.end());
  }
  record->parent.reset();
}

void Scene::destroy_subtree(const SceneNodeId node) {
  std::vector<SceneNodeId> stack;
  std::vector<SceneNodeId> post_order;
  stack.push_back(node);

  while (!stack.empty()) {
    const SceneNodeId current = stack.back();
    stack.pop_back();

    const NodeRecord* current_record = lookup(current);
    if (current_record == nullptr) {
      continue;
    }

    post_order.push_back(current);
    for (const SceneNodeId child : current_record->children) {
      stack.push_back(child);
    }
  }

  for (auto it = post_order.rbegin(); it != post_order.rend(); ++it) {
    NodeRecord* record = lookup(*it);
    if (record == nullptr) {
      continue;
    }

    detach_from_parent(*it);
    record = lookup(*it);
    if (record == nullptr) {
      continue;
    }

    record->alive = false;
    record->children.clear();
    record->parent.reset();
    record->camera.reset();
    record->light.reset();
    record->renderable.reset();
    record->vfx_attachment.reset();
    ++record->generation;
    free_indices_.push_back(it->index);
    RENDER_LOG_INFO("scene", "destroy node idx=", it->index);
  }
}

core::Transform Scene::relative_transform(const core::Transform& parent_world, const core::Transform& world) {
  core::Transform out{};
  out.rotation = conjugate(parent_world.rotation) * world.rotation;
  out.scale = safe_divide_components(world.scale, parent_world.scale);

  const core::Vec3 delta = world.translation - parent_world.translation;
  const core::Vec3 unrotated = rotate_vector(conjugate(parent_world.rotation), delta);
  out.translation = safe_divide_components(unrotated, parent_world.scale);
  return out;
}

}  // namespace render::scene
