# Ambient VFX System (Statement 15)

Statement 15 adds an engine-owned ambient VFX subsystem focused on stylized, lightweight atmosphere.

## Architecture

- Runtime module: `engine/render/vfx.hpp/.cpp`.
- Scene bridge: `scene::VfxAttachmentComponent` attaches a scene node to a VFX effect handle.
- Render integration: VFX produces instanced `DrawSubmission` payloads consumed by existing renderer submission APIs.

## Effect model

`EffectDefinition` defines effect templates with typed parameter groups:

- core: `EffectType`, `PrimitiveType`, `max_particles`, `spawn_rate_per_second`, `looping`
- spawn: `EffectSpawnRegion`
- particle motion: `ParticleStyle` (size/lifetime/velocity/drift/emissive)
- resin-specific: `ResinDripStyle`
- pulse-specific: `GlowPulseStyle`
- deterministic seed: `core::Seed`

Runtime state (`EffectState`) stores per-instance RNG, particle arrays, timers, and transform.

## CPU vs GPU responsibilities

Current maturity level intentionally uses a practical hybrid path:

- CPU: spawn/update/lifetime simulation and deterministic seeded randomization.
- GPU: draws all active particles/primitives through instanced sprite submissions.

This keeps costs low for ambient populations while avoiding premature compute complexity.

## Supported classes

Built-in definition helpers:

- spores (`make_spores_definition`)
- dust (`make_dust_definition`)
- embers (`make_embers_definition`)
- resin drips (`make_resin_drips_definition`)
- glow pulses (`make_glow_pulses_definition`)
- market hall ambience preset (`make_market_ambience_preset`)

Market ambience is composite by design (dust + warm specks + pulse accents).

## Renderer pass placement

`render_shell` submits VFX in an explicit `vfx-ambient-pass` after the main lit pass planning and before post placeholders. This keeps atmospheric sprites in the regular frame lifecycle and visible in renderer debug timings.

## Performance policy

- One instanced draw submission per active effect instance.
- Particle payload uploads are contiguous 4x4 transform arrays (`64 bytes/instance`).
- Toggle mask (`VfxToggleMask`) allows category-level disable for profiling.
- Debug counters include active effect/particle counts and VFX draw/upload totals.

## Current limits / deferred follow-ups

- No compute simulation yet.
- No per-instance color buffer (variant-based tinting used currently).
- Billboard facing/orientation is currently lightweight and world-oriented.
- Future statements can add richer resource bindings, textured sprites, collision, and gameplay-driven bursts.
