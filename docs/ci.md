# CI Foundation

## Workflows

- **PR Validation**: `.github/workflows/pr-validation.yml`
  - Runs on every pull request.
  - Provides fast cross-platform build + test confidence.
- **Main Validation**: `.github/workflows/main-validation.yml`
  - Runs on `main` pushes.
  - Re-runs cross-platform checks and full Linux packaging/shader gates.

## CI Matrix Policy

The matrix is intentionally split for speed and coverage:

- **Linux (full lane)**
  - Build
  - Unit tests
  - Headless smoke validation
  - Shader compilation checks (required)
  - Packaging validation + archive creation
- **macOS (selected lane)**
  - Build
  - Unit tests
  - Headless smoke validation
- **Windows (selected lane)**
  - Build
  - Unit tests
  - Headless smoke validation

Rationale: Linux provides fast and reproducible full validation, while macOS/Windows continuously enforce portability without tripling the most expensive checks.

## Dependency/bootstrap handling

CI uses the same CMake preset + dependency model as local development:

- Configure with repository presets (`linux-ci`, `macos-debug`, `windows-debug`, etc.)
- `RENDER_ALLOW_FETCHCONTENT=ON` for reproducible dependency acquisition
- `FETCHCONTENT_BASE_DIR=.fc` to keep dependency cache stable
- Ninja installed via `seanmiddleditch/gha-setup-ninja`

## Artifacts and logs

Linux full lanes upload:

- `out/build/<preset>/logs/` (shader + packaging logs)
- `out/build/<preset>/Testing/Temporary/` (CTest output)
- `out/build/<preset>/package/` (stage + generated archive)

This keeps failure triage practical without uploading oversized artifacts.

## Blocking failure policy

Blocking checks in CI:

- Configure/build failures
- Any failing unit/headless CTest case
- Shader compile failures in Linux full lanes
- Packaging layout/archive failures in Linux full lanes

All checks map to local commands documented in `docs/testing.md`.
