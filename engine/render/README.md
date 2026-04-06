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
