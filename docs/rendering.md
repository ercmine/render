# Renderer Runtime Lifecycle (Statement 9)

## Scope

Statement 9 hardens renderer runtime behavior around startup, backend selection, frame flow, resize handling, frame pacing, and recovery-oriented state management.

## Lifecycle model

`render::rendering::Renderer` now owns an explicit lifecycle state machine:

- `Uninitialized`
- `Initializing`
- `Ready`
- `Resizing`
- `Recovering`
- `ShuttingDown`
- `Failed`

The runtime shell and future systems can query:

- `state()`
- `is_ready()`
- `can_render()`
- `status()` (`RendererStatus` snapshot including frame stats)

The renderer logs every lifecycle transition.

## Startup and backend selection

Startup is configuration-driven via `RendererConfig`:

- `backend` (engine-owned enum, auto or explicit)
- `width`, `height`
- `debug`
- `vsync`
- `reset_flags` (engine-owned config mask)
- `min_frame_time_ms` (lightweight frame pacing floor)
- `allow_automatic_recovery`

Validation is explicit through `validate_renderer_config(...)`.

Backend behavior:

1. Engine backend enum maps to bgfx renderer type internally.
2. Requested backend and actual backend are both logged.
3. Explicit requests that are not selected log a warning and continue with the chosen backend.

## Frame lifecycle contract

Per-frame flow is now explicit and misuse-resistant:

1. `begin_frame()`
   - rejects invalid states
   - detects begin/begin misuse
   - applies deferred resize
   - returns `false` when rendering should be skipped (minimized/invalid state)
2. app records view setup + submissions
3. `end_frame()`
   - detects end without begin
   - presents (`bgfx::frame()`)
   - records frame stats and optional pacing sleep

This creates a stable contract for future view/pass expansion without redesigning loop ownership.

## Resize policy

Resize is explicit and resilient:

- shell forwards resize events with `request_resize(width, height)`
- resize requests are deduplicated and applied at frame boundary
- zero/minimized dimensions are treated as non-renderable and do not hard-fail the renderer
- rendering automatically resumes when a valid size arrives

## Frame pacing policy (current stage)

Current pacing is intentionally lightweight:

- primary pacing remains present/vsync (`vsync` + reset flags)
- optional minimum frame duration (`min_frame_time_ms`) prevents runaway present loops when desired
- no simulation scheduler or fixed-step governor is introduced at this stage

## Recovery/device-loss philosophy

A sane engine-owned recovery path is established:

- failed/recovery states are explicit
- `try_recover()` attempts an in-place reset path (`bgfx::reset`) using current backbuffer
- automatic recovery attempts are gated by `allow_automatic_recovery`
- recovery attempts and outcomes are logged

This is a practical base for later full resource re-creation pipelines.

## Runtime shell integration

`render_shell` now:

- forwards platform resize events through `request_resize`
- gates rendering work on `begin_frame()` success
- avoids undefined frame flow when minimized or otherwise non-renderable
- keeps shutdown ordered (`destroy resources` -> `renderer.shutdown()` -> `runtime.shutdown()`)

## Current non-goals / deferred work

Still deferred to later statements:

- full multi-device-loss backend edge-case matrix
- explicit persistent resource registry for automated rebuild
- render graph and multi-pass scheduling
- scene/material-driven rendering orchestration
