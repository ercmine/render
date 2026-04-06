[CmdletBinding()]
param ()

$ErrorActionPreference = 'Stop'

function Test-Tool {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $tool = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -ne $tool) {
        Write-Host "[ok] Found $Name"
        return $true
    }

    Write-Host "[warn] Missing $Name"
    return $false
}

Write-Host "== render bootstrap (Windows) =="
Write-Host "This script is non-destructive and only validates local tooling."
Write-Host ""

$allFound = $true
$allFound = (Test-Tool -Name 'cmake') -and $allFound
$allFound = (Test-Tool -Name 'ninja') -and $allFound
$allFound = (Test-Tool -Name 'git') -and $allFound
$allFound = (Test-Tool -Name 'cl') -or (Test-Tool -Name 'clang-cl') -or (Test-Tool -Name 'g++')

Write-Host ""
if (-not $allFound) {
    Write-Host 'One or more recommended tools are missing.'
    Write-Host 'Install missing tools (Visual Studio Build Tools or LLVM/MinGW as appropriate), then rerun.'
}
else {
    Write-Host 'All required baseline tools were found.'
}

Write-Host ""
Write-Host 'Next steps:'
Write-Host '  1) Configure: cmake --preset windows-debug'
Write-Host '  2) Build:     cmake --build --preset windows-debug'
Write-Host '  3) Release:   cmake --preset windows-release; cmake --build --preset windows-release'
Write-Host ''
Write-Host 'SDL3 and bgfx/bx/bimg are required for runtime targets. Configure via RENDER_SDL3_ROOT and RENDER_BGFX_ROOT, vendoring third_party/SDL3 and third_party/bgfx.cmake, or RENDER_ALLOW_FETCHCONTENT=ON.'
