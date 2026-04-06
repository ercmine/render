# engine/render

Engine-owned rendering interfaces and resource types.

## Boundary policy

- Public renderer API/types: `engine/render/*.hpp`
- bgfx-specific backend implementation: `engine/render/bgfx/*`

Higher-level engine and gameplay code should include only the public renderer headers.
