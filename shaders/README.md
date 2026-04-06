# shaders

Shader sources for the internal renderer live under `shaders/src/`.

## Current layout

- `src/varying.def.sc` — shared varying declarations
- `src/vs_statement4.sc` — Statement 4 vertex shader
- `src/fs_statement4.sc` — Statement 4 fragment shader

Compiled binaries are expected under `shaders/bin/<backend>/` at runtime (copied/generated into the build output under `bin/shaders/bin/<backend>/`).

Supported backend output folders at this stage:

- `spirv`
- `glsl`
- `metal`
- `dx11`

Use the `render_shaders` CMake target with `RENDER_BGFX_SHADERC` set to compile shader binaries.
