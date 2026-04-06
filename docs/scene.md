# Scene System (Statement 11)

## Module layout

- `engine/scene/scene.hpp`: public scene API and component data.
- `engine/scene/scene.cpp`: handle validation, hierarchy, dirty propagation, traversal, extraction.
- `engine/scene/README.md`: focused module notes.

The scene module is engine-owned and only references engine-owned core/render interfaces.

## Identity model

Each node uses `SceneNodeId { index, generation }`:

- `index` addresses storage slot
- `generation` invalidates stale handles after destroy/reuse

Destroyed nodes increment generation and return slot to a free-list. This prevents stale handles from silently targeting unrelated nodes.

## Transform + hierarchy rules

Each node stores:

- local transform (`core::Transform`)
- cached world transform (`core::Transform`)
- parent and children links

`Scene::update_world_transforms()` performs explicit top-down propagation from roots.

- Dirty state is pushed to descendants on local transform or hierarchy changes.
- Recompute is skipped for nodes that are not dirty and do not have dirty ancestry.

Reparenting policies:

- keep local: retain local transform values
- keep world: solve new local transform from current world and target parent world

Cycles are rejected with ancestor walk checks.

## Camera/light/renderable representation

### Cameras

`CameraComponent` supports:

- perspective + orthographic params (via `core::camera` types)
- enabled flag
- layer mask for filtering compatibility

`build_camera_view(aspect_ratio)` resolves active/fallback camera and returns derived view/projection matrices.

### Lights

`LightComponent` supports:

- directional / point types
- color, intensity, range
- enabled flag

Light direction/position come from node world transform.

### Renderables

`RenderableComponent` supports:

- mesh handle references
- program handle references
- draw state
- enabled flag + layer mask

Scene APIs keep renderer backend details hidden (no raw bgfx exposure).

## Visibility + debug names

Nodes expose:

- `NodeVisibility { enabled, layer_mask }`
- debug names as human-readable labels

Duplicate names are allowed. `find_nodes_by_name` returns all matches for tooling use.

## Renderer integration at this stage

`render_shell` now builds and renders through the scene:

- creates root/camera/light/renderable nodes
- parents nodes into hierarchy
- updates scene transforms each frame
- derives camera view/projection from scene
- collects visible renderables and submits to renderer

This validates the scene as the canonical runtime spatial layer.

## Current scope vs deferred

Implemented now:

- core node runtime, hierarchy, transforms, cameras/lights/renderables
- traversal/query/extraction helpers
- unit tests for handle, hierarchy, transform, components, visibility, naming

Deferred:

- binary/text scene serialization schema and IO flow
- culling structures and batching
- richer editor/runtime inspection APIs
