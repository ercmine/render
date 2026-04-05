# Dependency Strategy

Statement 3 integrates **SDL3** as the first runtime dependency. SDL3 is used for desktop platform services but is intentionally hidden behind engine-owned APIs under `engine/platform/`.

## Resolution order

`cmake/Dependencies.cmake` resolves SDL3 in this order:

1. **Explicit override path**: `RENDER_SDL3_ROOT`
2. **Vendored source**: `${RENDER_DEPENDENCY_ROOT}/SDL3` (default `third_party/SDL3`)
3. **System package**: `find_package(SDL3 CONFIG)`
4. **FetchContent fallback** (if `RENDER_ALLOW_FETCHCONTENT=ON`): pulls SDL from the official GitHub repo

If SDL3 cannot be resolved, configure fails with an actionable error.

## Notes

- `bgfx` is still scaffolded only (not integrated yet).
- SDL usage should stay in `engine/platform/sdl/*`.
- Higher-level engine/game modules should depend on `render::platform` / `render::engine`, not SDL directly.
