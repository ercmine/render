# render

`render` is a desktop-first C++20 codebase for building a custom 3D rendering engine and a game layer on top of it.

## Current milestone (Statement 8)

The repository now includes:

- an SDL3-backed platform runtime (`engine/platform`)
- an engine-owned renderer interface (`engine/render`)
- a bgfx/bx/bimg-backed renderer implementation hidden behind that interface
- a runnable shell that clears and submits minimal geometry each frame
- a deterministic serialization foundation for save, recipe, item, room, and network payload schemas
- an engine-owned filesystem/resource storage layer with category-based roots for source data, cache, saves, logs, screenshots, and crash dumps
- CTest-integrated unit + headless smoke validation entry points
- shader and packaging validation targets suitable for CI quality gates
- GitHub Actions workflows for PR and main-branch validation

## Key boundaries

- Engine/game modules should consume internal interfaces in `engine/platform/` and `engine/render/`.
- SDL3 usage is isolated to `engine/platform/sdl/` and renderer native-window glue.
- bgfx/bx/bimg usage is isolated to `engine/render/bgfx/`.

## Build quick start

Read `docs/build.md` for full setup.

Example (Linux Debug):

```bash
cmake --preset linux-debug -DRENDER_ALLOW_FETCHCONTENT=ON
cmake --build --preset linux-debug
./out/build/linux-debug/bin/render_shell
```

## Docs

- `docs/build.md` — build/configure/run commands
- `docs/dependencies.md` — dependency resolution and override strategy
- `docs/architecture.md` — platform + renderer boundary and layering notes
- `docs/serialization.md` — canonical binary encoding, schema/version policy, and migration hooks
- `docs/filesystem.md` — canonical path categories, writable layout, safe write behavior, and dev vs packaged lookup policy
- `docs/testing.md` — testing categories, labels, and local validation commands
- `docs/ci.md` — CI workflow matrix, blocking policy, and artifacts
