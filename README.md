# render

`render` is a desktop-first C++20 codebase for building a custom 3D rendering engine and a game layer on top of it.

## Current milestone (Statement 3)

The repository now includes a first runnable engine shell and a concrete platform layer backed by **SDL3**.

Implemented platform services (behind internal abstractions):

- application startup/shutdown
- window creation and lifecycle events
- event pumping
- keyboard + mouse input state collection
- gamepad/controller connect/disconnect + state updates
- timing/frame delta utilities
- base/pref/temp path queries
- basic audio stream plumbing for future PCM feeding

## Key boundaries

- Engine modules consume internal platform interfaces and types from `engine/platform/`.
- SDL3 is isolated to `engine/platform/sdl/` implementation code.

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
- `docs/dependencies.md` — SDL3 dependency resolution strategy
- `docs/architecture.md` — platform boundary and layering notes
