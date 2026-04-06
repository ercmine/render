# Testing and Validation

This repository uses four validation categories:

1. **Unit tests** (`unit` CTest label)
   - Fast deterministic tests for core/runtime, serialization, filesystem, and platform types.
2. **Headless validation** (`headless` CTest label)
   - Non-interactive smoke checks that validate startup/shutdown paths and key services without opening a graphics window.
3. **Shader compilation checks** (`render_shader_check` target)
   - Compiles required bgfx shaders and fails on compile errors.
4. **Packaging validation** (`render_package_validate` target)
   - Installs into a staging layout, verifies required files, and creates a package archive.

## CTest structure

Named tests follow `<category>.<domain>.<name>` to keep filtering predictable.

Current labels:

- `unit`
- `headless`

Examples:

```bash
ctest --test-dir out/build/linux-debug --output-on-failure -L unit
ctest --test-dir out/build/linux-debug --output-on-failure -L headless
```

## Local developer commands

Linux/macOS example:

```bash
cmake --preset linux-debug -DRENDER_ALLOW_FETCHCONTENT=ON
cmake --build --preset linux-debug
cmake --build --preset linux-debug --target render_test_unit
cmake --build --preset linux-debug --target render_test_headless
cmake --build --preset linux-debug --target render_shader_check
cmake --build --preset linux-debug --target render_package_validate
```

One-shot helper:

```bash
scripts/run_validations.sh linux-debug linux-debug -DRENDER_ALLOW_FETCHCONTENT=ON
```

Windows example:

```powershell
cmake --preset windows-debug -DRENDER_ALLOW_FETCHCONTENT=ON
cmake --build --preset windows-debug --config Debug
ctest --test-dir out/build/windows-debug -C Debug --output-on-failure -L unit
ctest --test-dir out/build/windows-debug -C Debug --output-on-failure -L headless
cmake --build --preset windows-debug --config Debug --target render_shader_check
cmake --build --preset windows-debug --config Debug --target render_package_validate
```

## Shader compilation policy

- `RENDER_REQUIRE_SHADER_COMPILATION=OFF` (default): shader check can be skipped locally if `shaderc` is unavailable; a warning is logged.
- `RENDER_REQUIRE_SHADER_COMPILATION=ON` (CI/full validation): missing `shaderc` or shader compile failures are blocking.
- If `shaderc` target exists from bgfx tools, it is used automatically; otherwise set `RENDER_BGFX_SHADERC` explicitly.

## Packaging validation policy

`render_package_validate` checks that stage/install output contains:

- `render_shell` executable
- `shaders/bin` directory
- at least one compiled shader binary (`*.bin`)
- generated package archive (`.tar.gz` on Unix, `.zip` on Windows)

Use `out/build/<preset>/logs/package_validation.log` to debug layout failures.
