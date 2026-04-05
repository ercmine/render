# Contributing to render

Thanks for helping build `render`.

## Repository Layout

Please review `README.md` and `docs/` before making structural changes. The project is intentionally separated into:

- `engine/` for reusable runtime systems
- `game/` for game-specific logic
- `tools/` for developer-facing utilities
- `shaders/` and `assets_recipes/` for rendering/content pipeline inputs
- `tests/` for validation coverage
- `third_party/` for external dependency code

## Branch Hygiene

- Prefer short-lived feature branches.
- Keep branch scope narrow and aligned with one objective.
- Rebase or merge frequently to avoid long-lived divergence.

## Keep Commits Focused

- Each commit should represent one logical change.
- Use clear commit messages that explain intent.
- Separate mechanical refactors from behavioral changes where possible.

## Do Not Commit Generated Outputs

- Do not commit binaries, compiled artifacts, caches, or local IDE state.
- Keep generated build outputs under ignored directories (for example within `build/`).

## Preserve Clean Module Boundaries

As modules are added, keep interfaces and responsibilities explicit:

- Avoid leaking game-specific concepts into `engine/`.
- Avoid coupling tools directly to runtime internals unless intentional and documented.
- Keep shader and asset recipe inputs data-oriented and organized by domain.
