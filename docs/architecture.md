# Planned Architecture

This document outlines the intended architectural direction for `render`.

## Design Goals

- Desktop-first custom 3D engine and game stack.
- Strong module boundaries between engine, game, and tooling.
- Data-driven and procedural workflows for content generation.
- Long-term maintainability through clear ownership and layering.

## Planned Layering

1. **Platform/Core Layer (`engine/platform`, `engine/core`)**
   - OS abstraction, lifecycle, memory/runtime services, and foundational utilities.
2. **Engine Systems Layer (`engine/render`, `engine/scene`, `engine/math`, `engine/procgen`, `engine/ui`)**
   - Rendering architecture, world/scene representation, math primitives, procedural generation support, and UI runtime support.
3. **Game Layer (`game/*`)**
   - Product-specific systems (colony simulation, entities, economy, market, progression, and persistence).
4. **Tooling Layer (`tools/*`)**
   - Pipeline and debugging tools that support authoring, validation, and iteration speed.
5. **Content Input Layer (`shaders/*`, `assets_recipes/*`)**
   - Shader source trees and recipe-driven data that feed runtime or build-time generation workflows.
6. **Verification Layer (`tests/*`)**
   - Unit and integration tests validating contracts across layers.

## Dependency Direction (Planned)

- Engine modules should remain reusable and game-agnostic.
- Game modules may depend on engine modules, not vice versa.
- Tools may depend on both engine and game representations when needed, but coupling should be explicit.
- Third-party code will be isolated under `third_party/` when introduced.

## Current State

This repository currently contains scaffolding only. Implementation details, build orchestration, and dependency integration are intentionally deferred.
