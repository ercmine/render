# Build Guide

## Supported Platforms

- Windows (MSVC/Clang-cl/MinGW toolchains through CMake)
- macOS (Clang)
- Linux (GCC or Clang)

## Required Tools

- CMake 3.25+
- Ninja (recommended default generator in presets)
- A C++20-capable compiler
- Git
- Dependency source for SDL3 + bgfx/bx/bimg (via the strategies below)

## Dependency options

Runtime dependencies can come from:

- override paths (`RENDER_SDL3_ROOT`, `RENDER_BGFX_ROOT`)
- vendored checkouts (`third_party/SDL3`, `third_party/bgfx.cmake`)
- SDL3 system package for SDL only (`find_package(SDL3 CONFIG)`)
- FetchContent fallback (`-DRENDER_ALLOW_FETCHCONTENT=ON`)

## Shader compilation options

- `render_shell` expects compiled shaders at runtime under `bin/shaders/bin/<backend>/`.
- The `render_shader_check` target drives shader compilation and validation.
- If `RENDER_BGFX_SHADERC` is not set, compilation is skipped unless `RENDER_REQUIRE_SHADER_COMPILATION=ON`.
- Enable `RENDER_BGFX_BUILD_TOOLS=ON` to build bgfx tooling (including `shaderc`) when available.

## Quick Start (Linux/macOS)

```bash
cmake --preset linux-debug -DRENDER_ALLOW_FETCHCONTENT=ON -DRENDER_BGFX_SHADERC=/path/to/shaderc
cmake --build --preset linux-debug
cmake --build --preset linux-debug --target render_shaders
./out/build/linux-debug/bin/render_shell
```

## Quick Start (Windows)

```powershell
cmake --preset windows-debug -DRENDER_ALLOW_FETCHCONTENT=ON -DRENDER_BGFX_SHADERC=C:/path/to/shaderc.exe
cmake --build --preset windows-debug --config Debug
cmake --build --preset windows-debug --config Debug --target render_shaders
.\out\build\windows-debug\bin\Debug\render_shell.exe
```

## Runtime shell behavior

`render_shell` opens an SDL window, initializes the engine renderer abstraction, clears every frame, and submits a minimal triangle when shader binaries are present.

Backend selection can be requested via:

- `--backend=noop`
- `--backend=d3d11`
- `--backend=d3d12`
- `--backend=metal`
- `--backend=vulkan`
- `--backend=opengl`

## Validation commands (local mirrors of CI)

```bash
cmake --build --preset linux-debug --target render_test_unit
cmake --build --preset linux-debug --target render_test_headless
cmake --build --preset linux-debug --target render_shader_check
cmake --build --preset linux-debug --target render_package_validate
cmake --build --preset linux-debug --target render_validate_all
```

See `docs/testing.md` and `docs/ci.md` for test category policy and CI matrix details.
