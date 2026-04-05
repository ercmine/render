#!/usr/bin/env sh
set -eu

log() {
  printf '%s\n' "$*"
}

check_cmd() {
  if command -v "$1" >/dev/null 2>&1; then
    log "[ok] Found $1"
  else
    log "[warn] Missing $1"
    MISSING_TOOLS=1
  fi
}

MISSING_TOOLS=0

log "== render bootstrap (macOS/Linux) =="
log "This script is non-destructive and only validates local tooling."

check_cmd cmake
check_cmd ninja
check_cmd git
check_cmd c++

log ""
if [ "$MISSING_TOOLS" -eq 1 ]; then
  log "One or more recommended tools are missing."
  log "Install the missing tools, then rerun this script."
else
  log "All required baseline tools were found."
fi

log ""
log "Next steps:"
log "  1) Configure: cmake --preset linux-debug   (or macos-debug)"
log "  2) Build:     cmake --build --preset linux-debug"
log "  3) Release:   cmake --preset linux-release && cmake --build --preset linux-release"
log ""
log "SDL3 is required for runtime targets. Configure via RENDER_SDL3_ROOT, vendoring third_party/SDL3, system package, or RENDER_ALLOW_FETCHCONTENT=ON."
