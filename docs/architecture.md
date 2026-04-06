# Architecture Snapshot (Statement 4)

## Runtime layering now implemented

1. **Platform layer (`engine/platform`)**
   - Engine-owned runtime API (`PlatformRuntime`) and data types (`platform_types.hpp`).
   - SDL3-backed implementation isolated to `engine/platform/sdl/platform_runtime_sdl.cpp`.
2. **Renderer layer (`engine/render`)**
   - Engine-owned renderer API, handles, view/buffer/shader/texture types.
   - bgfx-backed implementation isolated to `engine/render/bgfx/renderer_bgfx.cpp`.
3. **Engine layer (`render::engine`)**
   - Depends on platform and renderer abstractions only.
4. **Shell app (`render_shell`)**
   - Validates window + renderer lifecycle and submits a minimal draw path.

## Renderer responsibilities currently covered

- Startup/shutdown lifecycle and backend selection
- SDL native-window handoff to bgfx platform data
- Frame lifecycle (`begin_frame` / `end_frame`)
- View setup (rect + clear state + transforms)
- Vertex/index buffer creation and draw submission
- Shader program creation from compiled binaries
- Minimal bimg-powered texture creation path
- Basic startup diagnostics + debug text toggle

## Boundary rules

- SDL headers/functions are allowed in `engine/platform/sdl/*` and renderer backend glue only.
- bgfx/bx/bimg headers/functions are allowed only in `engine/render/bgfx/*`.
- Gameplay-facing and higher-level systems should consume `engine/render/renderer.hpp` and renderer-owned types.
