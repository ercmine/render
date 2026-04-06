# engine/render

Engine-owned rendering interfaces and resource types.

## Boundary policy

- Public renderer API/types: `engine/render/*.hpp`
- bgfx-specific backend implementation: `engine/render/bgfx/*`

Higher-level engine and gameplay code should include only the public renderer headers.

## Shader pipeline boundary (Statement 10)

- Engine-owned shader identity and lookup APIs live in `engine/render/shader_library.*`.
- Runtime systems should resolve shader programs through `ShaderProgramId` + `ShaderProgramLibrary`.
- bgfx shader compilation/tooling details remain in CMake scripts under `cmake/`.

## Lifecycle contract (Statement 9)

The public renderer API now exposes an explicit lifecycle and frame contract:

- Startup/shutdown: `initialize`, `shutdown`
- Query: `state`, `status`, `is_ready`, `can_render`
- Frame flow: `begin_frame`, `end_frame`
- Resize flow: `request_resize`, `resize`
- Recovery hook: `try_recover`

This keeps app/runtime code independent of bgfx lifecycle details.

## Geometry submission layer (Statement 12)

New engine-owned geometry submission files:

- `buffer_types.*`: vertex/index/instance descriptors, CPU mesh validation, material binding.
- `draw_submission.*`: draw submission records, batching keys, instancing policy, submission diagnostics.

`Renderer` now exposes mesh/index/instance buffer creation + update hooks and both unique and instanced submission entry points.

bgfx handles remain internal to backend implementation (`engine/render/bgfx/renderer_bgfx.cpp`).

## Lighting frame builder (Statement 13)

`lighting.hpp/.cpp` defines the engine-owned forward-plus style lighting planning layer.

- Input: visible directional lights, point lights, and renderable bounds/flags.
- Planning: capped light ranking and per-object light list generation.
- Output: deterministic packed CPU structures suitable for GPU upload in future passes.
- Controls: fog, bloom, shadows, and outlines with validation helpers.
- Diagnostics: selected/culled light counts and highlighted/shadowed object metrics.

This module intentionally keeps bgfx API details out of gameplay-facing code while preparing a scalable path to tiled/clustered improvements later.

## Renderer debug architecture (Statement 14)

`debug_renderer.hpp/.cpp` now owns renderer debug state and overlay formatting:

- canonical mode enum (`RendererDebugMode`) with runtime switching helpers
- frame counters (lights/draws/instances)
- pass timing samples (GPU optional, CPU fallback)
- overlay/HUD line generation for runtime shells and future tools
- program overrides used to translate regular scene submissions into debug views

Renderer-facing API hooks are exposed directly on `Renderer`:

- `set_debug_mode`, `set_debug_mode_from_index`, `cycle_debug_mode`
- `set_debug_program_overrides`
- `set_debug_counters`, `add_debug_pass_timing`
- `debug_snapshot` for tool/runtime inspection

Current debug view behavior:

- `wireframe`: backend wireframe flag wrapped by engine mode.
- `normals`: debug program override visualizing normalized direction.
- `albedo`: debug program override showing base vertex color.
- `depth`: debug program override showing remapped depth.
- `light-volumes`: normal shading with overlay diagnostics for light counts.
- `overdraw`: additive/no-depth approximation pass override.
- `gpu-timing`: overlay emphasizes per-pass timing rows; current implementation is CPU-timed with explicit `gpu=n/a` fallback.
