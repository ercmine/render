#include "engine/scene/scene.hpp"

#include <cassert>
#include <vector>

int main() {
  using namespace render;

  scene::Scene graph;

  const scene::SceneNodeId root = graph.create_node("root");
  const scene::SceneNodeId child = graph.create_node("child");
  const scene::SceneNodeId grandchild = graph.create_node("grandchild");

  assert(graph.is_alive(root));
  assert(graph.node_count() == 3);

  assert(graph.set_parent(child, root, scene::ReparentPolicy::KeepLocalTransform));
  assert(graph.set_parent(grandchild, child, scene::ReparentPolicy::KeepLocalTransform));
  assert(!graph.set_parent(root, grandchild));

  const std::vector<scene::SceneNodeId>* root_children = graph.children(root);
  assert(root_children != nullptr);
  assert(root_children->size() == 1);
  assert(root_children->front() == child);

  core::Transform root_transform{};
  root_transform.translation = {10.0F, 0.0F, 0.0F};
  assert(graph.set_local_transform(root, root_transform));

  core::Transform child_transform{};
  child_transform.translation = {0.0F, 5.0F, 0.0F};
  assert(graph.set_local_transform(child, child_transform));

  graph.update_world_transforms();
  const core::Transform* grandchild_world = graph.world_transform(grandchild);
  assert(grandchild_world != nullptr);
  assert(core::nearly_equal(grandchild_world->translation.x, 10.0F));
  assert(core::nearly_equal(grandchild_world->translation.y, 5.0F));

  assert(graph.clear_parent(child, scene::ReparentPolicy::KeepWorldTransform));
  graph.update_world_transforms();
  const core::Transform* child_local = graph.local_transform(child);
  assert(child_local != nullptr);
  assert(core::nearly_equal(child_local->translation.x, 10.0F));
  assert(core::nearly_equal(child_local->translation.y, 5.0F));

  scene::CameraComponent camera{};
  assert(graph.set_camera(root, camera));
  assert(graph.set_active_camera(root));
  const std::optional<scene::CameraView> camera_view = graph.build_camera_view(16.0F / 9.0F);
  assert(camera_view.has_value());

  scene::LightComponent light{};
  light.type = scene::LightType::Point;
  light.range = 3.0F;
  assert(graph.set_light(child, light));

  scene::RenderableComponent renderable{};
  renderable.mesh_buffer.idx = 7;
  renderable.index_buffer.idx = 8;
  renderable.material.program.idx = 11;
  renderable.index_count = 6;
  renderable.layer_mask = 0x1u;
  assert(graph.set_renderable(grandchild, renderable));

  const auto default_visible = graph.collect_visible_renderables(0x2u);
  assert(default_visible.empty());

  scene::NodeVisibility visible{};
  visible.layer_mask = 0x1u;
  assert(graph.set_visibility(grandchild, visible));
  const auto filtered_visible = graph.collect_visible_renderables(0x1u);
  assert(filtered_visible.size() == 1);

  assert(graph.set_debug_name(child, "duplicate"));
  assert(graph.set_debug_name(grandchild, "duplicate"));
  const auto duplicates = graph.find_nodes_by_name("duplicate");
  assert(duplicates.size() == 2);

  assert(graph.destroy_node(child));
  assert(!graph.is_alive(child));
  assert(!graph.is_alive(grandchild));

  const scene::SceneNodeId recycled = graph.create_node("recycled");
  assert(recycled.index == child.index);
  assert(recycled.generation != child.generation);
  assert(!graph.is_alive(child));

  return 0;
}
