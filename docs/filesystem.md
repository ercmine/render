# File and Resource Storage Model (Statement 7)

`engine/filesystem` defines the canonical storage and file-access layer for runtime, tools, and tests.

## Path categories

The service classifies paths by intent instead of passing ad-hoc raw paths around:

- `InstallRoot` — read-only install/base directory (typically from `SDL_GetBasePath` in runtime).
- `SourceData` — read-only development/package source content root (`assets/` in packaged builds; dev lookup can resolve to `assets_recipes/` when no `assets/` exists yet).
- `WritableRoot` — per-user writable root (typically from `SDL_GetPrefPath`).
- `Cache` — generated/disposable runtime cache root.
- `ShaderCache` — generated shader cache namespace (`cache/shaders/`).
- `GeneratedResources` — generated imported/processed resource outputs (`cache/generated/`).
- `Saves` — user-critical save files.
- `Logs` — runtime and diagnostics logs.
- `Screenshots` — exported screenshot files.
- `CrashDumps` — crash dump / minidump outputs.
- `Config` — persistent config/settings files.
- `Temp` — transient work files.
- `TestOutput` — deterministic test output root.

## Writable layout

Under `WritableRoot`, the runtime creates and owns this layout:

- `cache/`
  - `shaders/`
  - `generated/`
- `saves/`
- `logs/`
- `screenshots/`
- `crashes/`
- `config/`
- `temp/`
- `test_output/`

The service initializes these directories at startup and fails initialization if mandatory writable roots cannot be created.

## Development vs packaged lookup policy

Source data lookup order:

1. Explicit `source_root_override`.
2. Candidate paths relative to install root (`assets`, `assets_recipes`, and parent-relative variants).
3. Repo-root walk-up heuristic (`CMakeLists.txt` + `engine/` markers) to find `assets/` or `assets_recipes/` in development checkouts.
4. Fallback to `<install_root>/assets` (logged warning if missing).

Writable root lookup order:

1. Explicit `writable_root_override`.
2. Platform pref path (`SDL_GetPrefPath` via platform runtime).
3. If neither exists, initialization fails explicitly.

## Safe write policy

`WriteMode::AtomicReplace` uses temp-then-rename semantics in the same directory and is recommended for critical files:

- save files
- persistent config
- other user-critical deterministic blobs

`WriteMode::Direct` remains appropriate for disposable outputs like logs and many caches.

## API guidance

Use category-based calls:

- `root(PathCategory::Saves)`
- `resolve(PathCategory::Logs, "session/main.log")`
- `write_text(..., WriteMode::AtomicReplace)` for critical writes
- `make_cache_file_path(namespace, deterministic_key, extension)` for deterministic cache naming

Avoid scattering direct `std::filesystem` calls outside this layer for engine-owned file concerns.
