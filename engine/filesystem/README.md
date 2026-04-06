# engine/filesystem

Canonical engine-owned file and resource service.

This module provides:

- category-based path roots (`source`, `cache`, `saves`, `logs`, `screenshots`, `crash dumps`, etc.)
- path resolution with relative path validation
- directory bootstrapping and existence/type helpers
- text/binary read-write helpers
- safe atomic replace writes for user-critical files
- deterministic cache file naming helpers

Use this module instead of scattering raw `std::filesystem` usage for engine storage concerns.
