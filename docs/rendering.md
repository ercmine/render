# Rendering + Shader Pipeline (Statements 9-13)

## Scope

- Statement 9: renderer lifecycle hardening (startup/frame/resize/recovery).
- Statement 10: canonical shader source/build/runtime pipeline with variants, metadata, staging, and hot reload.
- Statement 11: scene graph integration (camera/light/renderable extraction).
- Statement 12: geometry submission + batching foundation.
- Statement 13: forward-plus style lighting data path (light selection, fog/bloom/shadow/outline settings, and diagnostics).

## Engine-owned shader architecture

The engine owns shader identity and runtime API through:

- `ShaderProgramId { category, name, variant }`
- `ShaderPipelineLayout` (runtime binary + metadata roots)
- `ShaderProgramLibrary` (load + hot reload policy)

bgfx shader tools remain implementation detail in build scripts.

## Shader source organization

See `shaders/README.md` for full conventions. High-level structure:

- `shaders/includes/` shared include files and varying definitions
- `shaders/materials/` material-domain programs
- `shaders/post/` post-processing programs
- `shaders/debug/` debug visualization programs
- `shaders/shaders.cmake` manifest of program entries and variants

This organization is designed to scale without introducing a custom shader DSL.

## Build graph integration

CMake target flow:

- `render_shaders` compiles all manifest entries and emits reflection metadata.
- `render_shader_check` depends on `render_shaders`.
- `render_shell` depends on `render_shaders` (runtime always uses generated artifacts).
- `render_package_stage` and `render_package_validate` include shader binaries and metadata.

Primary knobs:

- `RENDER_BGFX_SHADERC`
- `RENDER_SHADER_BACKENDS`
- `RENDER_SHADER_OUTPUT_ROOT`
- `RENDER_REQUIRE_SHADER_COMPILATION`

## Backend/platform output handling

Shader outputs are generated under:

- `bin/shaders/bin/<backend>/<category>/<program>/<variant>/<stage>.bin`
- `bin/shaders/metadata/<backend>/<category>/<program>/<variant>/*.json`

Backend mapping is explicit in compile scripts (`spirv`, `glsl`, `metal`, `dx11`) and runtime lookup uses the same mapping through `backend_shader_folder(...)`.

## Runtime loading and hot reload

`render_shell` now loads the debug triangle program through `ShaderProgramLibrary` instead of hardcoded shader file paths.

Hot reload policy:

- monitor compiled vertex/fragment binaries and `program.json`
- if changed, attempt loading replacement program
- only swap handle after successful creation
- on failure, retain previous valid program and log warning

This keeps development iteration fast while avoiding renderer corruption on bad recompiles.

## Reflection metadata foundation

The pipeline emits engine-facing JSON metadata:

- stage metadata (`vs.json`, `fs.json`) with identity, source, defines, backend, and SHA256
- `program.json` with stage list and binary/metadata references

This is intentionally a foundation for later material binding validation and pipeline introspection.

## Current limits / follow-ups

Deferred to later statements:

- richer uniform/resource reflection extraction beyond identity/hash fields
- compute shader runtime integration (manifest structure already reserves this)
- centralized material system on top of `ShaderProgramId`
- full editor/content pipeline tooling around variants and dependency graphs


## Scene integration (Statement 11)

`render_shell` now drives rendering through `engine/scene`:

- scene nodes carry camera/light/renderable attachments
- frame flow calls `Scene::update_world_transforms()` before extraction
- view matrices come from `Scene::build_camera_view(...)`
- draw submissions are emitted from `Scene::collect_visible_renderables(...)`

This establishes a stable engine-owned scene/runtime boundary above the renderer API.

## Geometry submission foundation (Statement 12)

Statement 12 introduces an engine-owned geometry submission path designed for procedural workloads.

### Implemented renderer-side concepts

- `VertexLayoutDescription` with explicit offsets/stride and validation.
- `MeshBufferDescription` / `IndexBufferDescription` with usage intent and debug metadata.
- `InstanceBufferDescription` abstraction for reusable instance payload uploads.
- CPU mesh bridge through `CpuMeshData` + `validate_cpu_mesh_data(...)`.
- `MaterialBinding` wrapping program + parameter hash (with texture slots reserved).
- `DrawSubmission` and `DrawBatch` structures for explicit per-draw/per-batch inspection.

### Batching rules

Current batching key is the tuple:

- view id
- mesh buffer handle
- index buffer handle
- program handle
- render state flags
- material parameter hash

Draws sharing the same key are grouped together. If group size is >= `BatchPolicy::min_instanced_count` (default 4), the renderer emits instanced draws; otherwise each draw is submitted uniquely.

This creates deterministic, stable grouping and keeps a clear path for future auto-batching improvements.

### Instancing policy

- Preferred path for repeated procedural objects is instancing.
- One-off or small groups remain unique draws.
- `BatchPolicy::max_instances_per_draw` caps batch size and splits very large groups into multiple instanced draw calls.

### Scene integration

Scene renderables now carry renderer-owned resource bindings:

- `mesh_buffer` + optional `index_buffer/index_count`
- `material` (`MaterialBinding`)
- `draw_state`

`render_shell` extracts scene-visible renderables, creates `DrawSubmission` records, runs batch planning, and submits unique or instanced draws via renderer API.

### Diagnostics

`SubmissionDiagnostics` reports:

- submitted draw count
- instanced draw count
- unique draw count
- submitted instance total
- batch count

These diagnostics are intended for profiling/debug overlays and frame-level instrumentation.

### Deferred follow-ups

- Dynamic GPU buffer update path currently uses a placeholder API contract and is staged for a follow-up statement.
- Full material parameter/uniform binding and texture set binding beyond fixed slots is deferred.
- GPU-driven culling/indirect draw remains deferred by design.

## Forward-plus style lighting path (Statement 13)

Statement 13 introduces the first engine-owned lighting frame builder under `engine/render/lighting.*`.

### What \"forward-plus style\" means in this repo right now

This is a **forward renderer with explicit light selection**, not a full clustered/tiled implementation yet.

Current strategy:

1. Collect scene lights per-view (`Scene::collect_visible_lights`).
2. Rank point lights by camera-relative importance (intensity + range-weighted distance).
3. Keep only a capped point-light set (`LightingSelectionConfig::max_point_lights`, hard-clamped by `kMaxPointLights`).
4. Build per-object point-light lists capped by `max_lights_per_object`.
5. Upload deterministic packed arrays (`DirectionalLightGpu`, `PointLightGpu`, `ObjectLightList`) to the rendering path.

This avoids unbounded \"every object loops every light\" behavior while preserving a clean extension path toward tiled/clustered lists later.

### Implemented engine-facing controls

- Directional lights (color/intensity/direction, shadow flag).
- Point lights (position/color/intensity/range, shadow flag placeholder).
- Fog settings (`FogSettings`) with linear/exp/exp2 modes and validation.
- Bloom settings (`BloomSettings`) with threshold/intensity/downsample controls and validation.
- Directional shadow settings (`ShadowSettings`) with map size + bias/filter defaults and validation.
- Outline settings (`OutlineSettings`) for stylized highlight composition controls.
- Diagnostics (`LightingDiagnostics`) for selected/culled counts and highlight/shadow stats.

### Current integration state

- Scene components now include `RenderableComponent` emissive/highlight fields and `LightComponent::casts_shadows`.
- Scene-level lighting knobs are stored in `SceneLightingSettings` (`fog` + `bloom`) and consumed by the shell.
- `render_shell` now builds a stylized validation setup with directional + multiple point lights, emissive/highlighted renderables, and lighting frame extraction each frame.

### Deferred to follow-up statements

- GPU-side uniform/buffer binding of packed lighting data to lit shaders.
- Dedicated shadow-map render passes and shader sampling.
- Bloom extraction/blur/composite and outline composition passes.
- Full lit material shader programs (current shell still renders via debug triangle program).
- Debug view rendering modes (normals, emissive-only, bloom extraction, shadow atlas visualization).
