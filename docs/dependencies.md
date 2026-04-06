# Dependency Strategy

Statement 4 integrates **SDL3** and the **bgfx stack** (**bgfx + bx + bimg**) for the renderer foundation.

## Resolution order

`cmake/Dependencies.cmake` resolves dependencies in this order:

### SDL3

1. **Explicit override path**: `RENDER_SDL3_ROOT`
2. **Vendored source**: `${RENDER_DEPENDENCY_ROOT}/SDL3` (default `third_party/SDL3`)
3. **System package**: `find_package(SDL3 CONFIG)`
4. **FetchContent fallback** (`RENDER_ALLOW_FETCHCONTENT=ON`)

### bgfx/bx/bimg

1. **Explicit override path**: `RENDER_BGFX_ROOT` (expects a `bgfx.cmake` checkout)
2. **Vendored source**: `${RENDER_DEPENDENCY_ROOT}/bgfx.cmake` (default `third_party/bgfx.cmake`)
3. **FetchContent fallback** (`RENDER_ALLOW_FETCHCONTENT=ON`) from pinned `bgfx.cmake` commit

The `bgfx.cmake` integration builds bgfx together with bx/bimg and exports CMake targets consumed by `render::dependencies`.

## Notes

- Gameplay-facing code must not include SDL3/bgfx headers directly.
- SDL usage is isolated to `engine/platform/sdl/*`.
- bgfx/bx/bimg usage is isolated to `engine/render/bgfx/*`.
- Higher-level engine/game modules should depend on engine-owned interfaces in `engine/platform/` and `engine/render/`.
