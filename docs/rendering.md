# Renderer Integration (Statement 4)

## Scope

Statement 4 introduces a renderer foundation powered by bgfx/bx/bimg but exposed through engine-owned interfaces.

## Where code lives

- Public interfaces and engine-owned types: `engine/render/*.hpp`
- bgfx backend implementation: `engine/render/bgfx/renderer_bgfx.cpp`
- Shader sources: `shaders/src/*.sc`
- Shader build hook: `cmake/ShaderCompilation.cmake`, `cmake/compile_shaders.cmake`

## Allowed direct third-party usage

- `engine/render/bgfx/*`: may include and call bgfx/bx/bimg APIs
- `engine/platform/sdl/*`: may include and call SDL APIs

All other engine/gameplay modules should stay on engine-owned types.

## SDL window handoff

Renderer initialization receives `PlatformRuntime` and reads the SDL window pointer through `native_window_handle()`.
The bgfx backend extracts native platform handles from SDL window properties and fills `bgfx::Init::platformData`.

## Current renderer API coverage

- lifecycle (`initialize`, `shutdown`)
- backend selection (`RendererBackend`)
- frame flow (`begin_frame`, `end_frame`, `resize`)
- view setup (`set_view`, `set_view_transform`)
- resource creation (`create_vertex_buffer`, `create_index_buffer`, `create_program`, `create_solid_color_texture`)
- draw submission (`submit`)
- diagnostics (`capabilities`, debug text)

## Current validation target

`render_shell` now:

1. starts SDL runtime/window
2. initializes renderer with config
3. clears the main view every frame
4. submits a simple indexed triangle when shader binaries are available
5. handles resize and clean shutdown

## Follow-up work (later statements)

- richer resource lifetime/allocator systems
- robust shader pipeline tooling and packaging
- material and scene rendering systems
- texture/image asset loading pipeline
- render graph/pass scheduling
