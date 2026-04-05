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
- SDL3 dependency source (resolved by one of the dependency strategies below)

## SDL3 dependency options

SDL3 can come from:

- `RENDER_SDL3_ROOT` override path
- vendored checkout in `third_party/SDL3`
- system `find_package(SDL3 CONFIG)` installation
- FetchContent fallback (set `-DRENDER_ALLOW_FETCHCONTENT=ON`)

## Quick Start (Linux/macOS)

```bash
cmake --preset linux-debug -DRENDER_ALLOW_FETCHCONTENT=ON
cmake --build --preset linux-debug
ctest --test-dir out/build/linux-debug --output-on-failure
./out/build/linux-debug/bin/render_shell
```

## Quick Start (Windows)

```powershell
cmake --preset windows-debug -DRENDER_ALLOW_FETCHCONTENT=ON
cmake --build --preset windows-debug --config Debug
ctest --test-dir out/build/windows-debug -C Debug --output-on-failure
.\out\build\windows-debug\bin\Debug\render_shell.exe
```

## Runtime shell behavior

`render_shell` opens a resizable window, pumps events, tracks input/gamepads/timing, initializes audio plumbing, and exits when the window is closed.
