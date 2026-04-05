# Build Guide

## Supported Platforms

- Windows (MSVC/Clang-cl/MinGW toolchains through CMake)
- macOS (Clang)
- Linux (GCC or Clang)

## Required Tools

- CMake 3.25+
- Ninja (recommended default generator in presets)
- A C++20-capable compiler
  - Windows: Visual Studio Build Tools, clang-cl, or MinGW
  - macOS: Apple Clang (Xcode Command Line Tools)
  - Linux: GCC or Clang
- Git

## Quick Start

1. Run platform bootstrap script (non-destructive):
   - macOS/Linux: `./scripts/bootstrap.sh`
   - Windows PowerShell: `./scripts/bootstrap.ps1`
2. Configure with a preset.
3. Build with the matching build preset.

## Presets

### Configure presets

- `windows-debug`
- `windows-release`
- `linux-debug`
- `linux-release`
- `macos-debug`
- `macos-release`

### Build presets

- `windows-debug`
- `windows-release`
- `linux-debug`
- `linux-release`
- `macos-debug`
- `macos-release`

## Common Commands

```bash
# Linux/macOS debug
cmake --preset linux-debug
cmake --build --preset linux-debug

# Linux/macOS release
cmake --preset linux-release
cmake --build --preset linux-release

# Windows debug
cmake --preset windows-debug
cmake --build --preset windows-debug

# Windows release
cmake --preset windows-release
cmake --build --preset windows-release
```

## Build Output Layout

All generated build trees are out-of-source under:

- `out/build/<preset-name>/`

Typical artifact directories inside each build tree:

- `bin/`
- `lib/`

## Configuration Options (initial)

- `RENDER_BUILD_TOOLS` (default `ON`)
- `RENDER_BUILD_TESTS` (default `ON`)
- `RENDER_WARNINGS_AS_ERRORS` (default `OFF`)
- `RENDER_ENABLE_SANITIZERS` (default `OFF`)
- `RENDER_ENABLE_UNITY_BUILD` (default `OFF`)
- `RENDER_ENABLE_IPO` (default `OFF`)

## Notes

This statement establishes build infrastructure and placeholder targets only. Real engine/game implementations and runtime dependencies are integrated in later statements.
