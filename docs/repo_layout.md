# Repository Layout

This repository is organized to separate runtime, game logic, content pipelines, tooling, and verification.

## Top-Level Directories

- `engine/`  
  Custom engine systems and reusable runtime modules.
- `game/`  
  Game-specific features built on engine interfaces.
- `tools/`  
  Internal command-line and desktop utilities for content and debugging workflows.
- `shaders/`  
  Shader source (`src/`) and compiled backend binaries (`bin/<backend>/`).
- `third_party/`  
  Reserved for external dependency code, mirrors, or vendor drops.
- `tests/`  
  Testing hierarchy (`unit`, `integration`) for verification at multiple levels.
- `assets_recipes/`  
  Programmatic/procedural recipe definitions for game and visual content.
- `build/`  
  Build outputs and generated artifacts (kept mostly untracked).
- `docs/`  
  Architectural intent and repository conventions.

## Planned Internal Organization

### `engine/`
- `platform/`, `render/`, `scene/`, `math/`, `core/`, `procgen/`, `ui/`

### `game/`
- `colony/`, `bugs/`, `items/`, `economy/`, `expeditions/`, `market/`, `save/`

### `tools/`
- `recipe_viewer/`, `shader_build/`, `mesh_debug/`

### `tests/`
- `unit/`, `integration/`

### `assets_recipes/`
- `rooms/`, `bugs/`, `items/`, `materials/`, `ui/`

### `shaders/`
- `includes/`, `materials/`, `post/`, `debug/`

## Notes

Layout is incrementally populated by statements. Platform + renderer foundations are now active under `engine/platform` and `engine/render`.
