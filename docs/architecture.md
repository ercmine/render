# Architecture Snapshot (Statement 3)

## Runtime layering now implemented

1. **Platform layer (`engine/platform`)**
   - Engine-owned runtime API (`PlatformRuntime`) and data types (`platform_types.hpp`).
   - SDL3-backed implementation isolated to `engine/platform/sdl/platform_runtime_sdl.cpp`.
2. **Engine layer (`render::engine`)**
   - Depends on platform abstraction only.
3. **Shell app (`render_shell`)**
   - Minimal runnable loop that initializes runtime, pumps events, updates timing/input, and exits on close.

## Platform responsibilities currently covered

- Startup/shutdown lifecycle
- Window create + lifecycle events
- Event pump
- Keyboard + mouse input state tracking (down/pressed/released, text input, mouse wheel)
- Gamepad connect/disconnect + state updates
- Timing (frame delta + monotonic counters)
- Path queries (base/pref/temp)
- Audio bootstrap with default playback stream and sample queue plumbing

## Boundary rule

SDL headers/functions are allowed in `engine/platform/sdl/*` only.
All other modules must consume the engine-owned platform interfaces and types.
