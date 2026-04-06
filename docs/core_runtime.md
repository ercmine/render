# Core Runtime Layer (Statement 5)

The `engine/core` module is the engine-owned foundation for all runtime systems.

## What belongs in `engine/core`

- Low-level value types and constants (`types.hpp`)
- Math primitives (`math.hpp`) and transform/camera helpers
- Color utilities and color-space conversion helpers
- Deterministic identity/runtime utilities (UUID, hashing, seeds, RNG)
- Allocator and threading wrappers with light abstraction
- Shared diagnostics: logging, assertions, and profiling hooks

`engine/core` intentionally avoids gameplay and scene orchestration logic.

## Conventions

### Math, transforms, and camera

- Matrices are `Mat4` in column-major memory layout.
- Transform composition uses `T * R * S` order.
- Right-handed camera helpers are provided (`make_look_at`, perspective and orthographic projection builders).
- Floating-point equality should use epsilon-aware comparisons (`nearly_equal`).

### Color

- `ColorRGBA` uses normalized floats in `[0, 1]`.
- `ColorRGBA8` stores 8-bit packed channels.
- `srgb_to_linear` and `linear_to_srgb` cover standard transfer behavior.

## Determinism policy

- `hash.hpp` provides stable FNV-1a (64-bit) utilities for runtime IDs and recipe keys.
- `Seed` and `Random` avoid hidden global RNG state.
- Child seeds are explicitly derived (`derive_seed`, `Random::split`) for reproducible procgen branching.
- UUID generation (`Uuid::generate_v4`) is intentionally non-deterministic and reserved for external identity, not simulation logic.

## Logging / assertions / profiling usage

- Use `RENDER_LOG_*` macros with a category string (`"renderer"`, `"procgen"`, etc.).
- Use `RENDER_ASSERT` for debug-only invariants.
- Use `RENDER_VERIFY` for release-safe checks that should log on failure.
- Use `RENDER_PROFILE_SCOPE("name")` and `RENDER_PROFILE_FRAME("frame")` as hook points.
- Profiling macros become no-ops unless `RENDER_ENABLE_PROFILING` is enabled.

## Memory / threading philosophy (current stage)

- Keep standard library types first-class.
- `IAllocator` + `default_allocator()` establish a clean extension point without forcing immediate custom allocator adoption.
- Thread wrappers are intentionally thin and instrumentation-friendly.
