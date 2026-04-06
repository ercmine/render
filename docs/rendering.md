# Rendering + Shader Pipeline (Statements 9-10)

## Scope

- Statement 9: renderer lifecycle hardening (startup/frame/resize/recovery).
- Statement 10: canonical shader source/build/runtime pipeline with variants, metadata, staging, and hot reload.

## Engine-owned shader architecture

The engine owns shader identity and runtime API through:

- `ShaderProgramId { category, name, variant }`
- `ShaderPipelineLayout` (runtime binary + metadata roots)
- `ShaderProgramLibrary` (load + hot reload policy)

bgfx shader tools remain implementation detail in build scripts.

## Shader source organization

See `shaders/README.md` for full conventions. High-level structure:

- `shaders/includes/` shared include files and varying definitions
- `shaders/materials/` material-domain programs
- `shaders/post/` post-processing programs
- `shaders/debug/` debug visualization programs
- `shaders/shaders.cmake` manifest of program entries and variants

This organization is designed to scale without introducing a custom shader DSL.

## Build graph integration

CMake target flow:

- `render_shaders` compiles all manifest entries and emits reflection metadata.
- `render_shader_check` depends on `render_shaders`.
- `render_shell` depends on `render_shaders` (runtime always uses generated artifacts).
- `render_package_stage` and `render_package_validate` include shader binaries and metadata.

Primary knobs:

- `RENDER_BGFX_SHADERC`
- `RENDER_SHADER_BACKENDS`
- `RENDER_SHADER_OUTPUT_ROOT`
- `RENDER_REQUIRE_SHADER_COMPILATION`

## Backend/platform output handling

Shader outputs are generated under:

- `bin/shaders/bin/<backend>/<category>/<program>/<variant>/<stage>.bin`
- `bin/shaders/metadata/<backend>/<category>/<program>/<variant>/*.json`

Backend mapping is explicit in compile scripts (`spirv`, `glsl`, `metal`, `dx11`) and runtime lookup uses the same mapping through `backend_shader_folder(...)`.

## Runtime loading and hot reload

`render_shell` now loads the debug triangle program through `ShaderProgramLibrary` instead of hardcoded shader file paths.

Hot reload policy:

- monitor compiled vertex/fragment binaries and `program.json`
- if changed, attempt loading replacement program
- only swap handle after successful creation
- on failure, retain previous valid program and log warning

This keeps development iteration fast while avoiding renderer corruption on bad recompiles.

## Reflection metadata foundation

The pipeline emits engine-facing JSON metadata:

- stage metadata (`vs.json`, `fs.json`) with identity, source, defines, backend, and SHA256
- `program.json` with stage list and binary/metadata references

This is intentionally a foundation for later material binding validation and pipeline introspection.

## Current limits / follow-ups

Deferred to later statements:

- richer uniform/resource reflection extraction beyond identity/hash fields
- compute shader runtime integration (manifest structure already reserves this)
- centralized material system on top of `ShaderProgramId`
- full editor/content pipeline tooling around variants and dependency graphs
