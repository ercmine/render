# Scene Runtime (Statement 11)

`engine/scene` provides the engine-owned spatial scene foundation.

## Current capabilities

- Stable scene node handles with generation checks (`SceneNodeId`).
- Parent-child hierarchy with cycle prevention and explicit reparent policy.
- Local + world transforms using `core::Transform` and explicit `update_world_transforms()`.
- Optional node attachments for camera, light, and renderable data.
- Node visibility + layer masks and renderable-specific layer masks.
- Debug names and duplicate-name lookup.
- Scene traversal and renderer-facing extraction helpers (`build_camera_view`, `collect_visible_renderables`, `collect_visible_lights`).

## Update model

The scene does not mutate world transforms implicitly for every operation. Call `Scene::update_world_transforms()` after structural/transform edits and before extraction/submission.

Dirty propagation rules:

- writing local transform marks node subtree dirty
- reparent/unparent marks affected subtree dirty
- update pass recomputes world transforms top-down from roots

## Reparent policy

`set_parent` and `clear_parent` take `ReparentPolicy`:

- `KeepLocalTransform`: preserve local transform fields, world changes relative to new parent
- `KeepWorldTransform`: preserve world transform and recompute local relative to new parent

## Visibility rules

A node contributes renderables/lights only when:

- node visibility `enabled == true`
- component `enabled == true`
- node and component layer masks overlap caller-provided layer mask

## Deferred for later statements

- scene serialization format + load/save pipelines
- culling acceleration structures
- advanced camera controllers and camera blending
- shadow map authoring and broader light model
- material graph / resource binding systems
