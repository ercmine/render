#!/usr/bin/env bash
set -eu

PRESET="${1:-linux-debug}"
BUILD_PRESET="${2:-$PRESET}"

log() {
  printf '%s\n' "$*"
}

log "== render validation pipeline =="
log "Configure preset: ${PRESET}"
log "Build preset: ${BUILD_PRESET}"

cmake --preset "${PRESET}" "${@:3}"
cmake --build --preset "${BUILD_PRESET}" --target render_shader_check
ctest --preset "${PRESET}" --output-on-failure --label-regex "unit|headless"
cmake --build --preset "${BUILD_PRESET}" --target render_package_validate

log "All validation categories completed."
