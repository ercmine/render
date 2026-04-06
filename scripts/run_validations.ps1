param(
  [string]$Preset = "windows-debug",
  [string]$BuildPreset = "windows-debug",
  [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

Write-Host "== render validation pipeline =="
Write-Host "Configure preset: $Preset"
Write-Host "Build preset: $BuildPreset"

cmake --preset $Preset
cmake --build --preset $BuildPreset --config $Config --target render_shader_check
ctest --test-dir "out/build/$Preset" -C $Config --output-on-failure -L unit|headless
cmake --build --preset $BuildPreset --config $Config --target render_package_validate

Write-Host "All validation categories completed."
