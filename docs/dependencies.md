# Dependency Strategy (Bootstrap Phase)

This repository uses an incremental dependency bootstrap model so new contributors can configure builds consistently before full third-party integration is complete.

## Current Phase

- No major runtime dependencies are fully integrated yet (for example SDL3/bgfx are intentionally not wired in this statement).
- `cmake/Dependencies.cmake` provides extension points and cache variables for future dependency registration.

## Supported Dependency Sources (design)

The intended resolution order is:

1. **Explicit local override paths**
   - Set through cache variables (often from user presets):
     - `RENDER_SDL3_ROOT`
     - `RENDER_BGFX_ROOT`
2. **Vendored source trees**
   - Conventional location: `third_party/<name>`
   - Global root override: `RENDER_DEPENDENCY_ROOT`
3. **Optional FetchContent fallback**
   - Controlled by `RENDER_ALLOW_FETCHCONTENT`
   - Mechanism is scaffolded; concrete dependency fetch rules will be added in later statements.

## Why this approach

- Keeps the initial build deterministic and simple.
- Supports both enterprise/offline and open-source workflows.
- Avoids a redesign when real dependencies are introduced.

## Contributor Workflow (today)

1. Clone the repository.
2. Run `scripts/bootstrap.sh` (macOS/Linux) or `scripts/bootstrap.ps1` (Windows).
3. Configure using one of the standard presets from `CMakePresets.json`.
4. Optionally create a local `CMakeUserPresets.json` from `CMakeUserPresets.json.example` and set machine-specific dependency overrides.

## Planned follow-up work

- Add concrete SDL3 and bgfx registration logic in `cmake/Dependencies.cmake`.
- Decide per-library policy (submodule vs FetchContent vs system package).
- Add lock/version strategy for reproducible third-party revisions.
