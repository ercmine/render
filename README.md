# render

`render` is a desktop-first C++20 codebase for building a custom 3D rendering engine and a game layer on top of it.

This repository is intentionally focused on long-term maintainability and clear separation of responsibilities between foundational engine systems, game-specific logic, tooling, and content pipelines.

## Project Intent

- **Platform target:** Desktop-first development targeting **Windows, macOS, and Linux**.
- **Technology direction:** **C++20** with a **custom engine architecture**.
- **Visual/content direction:** Programmatic and procedural graphics workflows, supported by asset recipes and internal tooling.
- **Current milestone:** **Repository scaffolding only** (no renderer/gameplay/build implementation yet).

## High-Level Architecture (Planned)

The project is structured in layers:

1. **Engine (`engine/`)**
   - Cross-cutting runtime systems such as platform abstraction, rendering, scene management, math, core utilities, procedural generation, and UI foundations.
2. **Game (`game/`)**
   - Domain/gameplay modules built on top of the engine, isolated from low-level engine internals.
3. **Tools (`tools/`)**
   - Developer-facing utilities for content preview, shader workflows, and debugging.
4. **Content Pipelines (`shaders/`, `assets_recipes/`)**
   - Shader source organization and data-driven/procedural recipe definitions for generating runtime content.
5. **Verification (`tests/`)**
   - Unit and integration testing for engine and game modules.
6. **External Dependencies (`third_party/`)**
   - Reserved location for vendored or mirrored external code (to be integrated later).

## Repository Layout

- `engine/` — Core custom engine modules.
- `game/` — Game-specific modules and systems.
- `tools/` — Internal developer tools and pipeline helpers.
- `shaders/` — Shader code, includes, and rendering passes.
- `third_party/` — Placeholder for external dependencies.
- `tests/` — Unit/integration test organization.
- `assets_recipes/` — Procedural/programmatic asset recipe sources.
- `build/` — Build output staging and generated artifacts (ignored in git where appropriate).
- `docs/` — Architecture and repository design documentation.

## Status

This first commit establishes only the initial structure and documentation to support future implementation phases. No production renderer, gameplay systems, dependency setup, or full build logic is included yet.
