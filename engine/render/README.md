# engine/render

Engine-owned rendering interfaces and resource types.

## Boundary policy

- Public renderer API/types: `engine/render/*.hpp`
- bgfx-specific backend implementation: `engine/render/bgfx/*`

Higher-level engine and gameplay code should include only the public renderer headers.

## Lifecycle contract (Statement 9)

The public renderer API now exposes an explicit lifecycle and frame contract:

- Startup/shutdown: `initialize`, `shutdown`
- Query: `state`, `status`, `is_ready`, `can_render`
- Frame flow: `begin_frame`, `end_frame`
- Resize flow: `request_resize`, `resize`
- Recovery hook: `try_recover`

This keeps app/runtime code independent of bgfx lifecycle details.
