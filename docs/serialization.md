# Deterministic Serialization (Statement 6)

`render` uses an engine-owned canonical binary format for all persisted/network payload foundations.

## Canonical encoding rules

- Endianness: **little-endian** for all integer and IEEE-754 floating-point bit patterns.
- Integer widths: explicitly encoded as `u8/u16/u32/u64` and `i32/i64`.
- Booleans: single byte (`0` or `1` only).
- Strings: UTF-8 bytes encoded as `u32 length` + raw bytes.
- Blobs: `u32 length` + raw bytes.
- Arrays/vectors: `u32 count` + element stream in declared order.
- Optional values: `bool present` + value when present.
- Enums: explicit `u32` storage unless a schema defines another explicit width.
- No direct struct memory dumps; each field is serialized explicitly and in declared field order.

## Schema identity and versioning

Every top-level serialized object uses an `ObjectEnvelope`:

- magic (`0x4A42534F`) and envelope format version
- schema ID (`u32`)
- schema version (`u16`)
- flags (`u32`, reserved for future compression/encryption/checksum routing)
- payload size (`u32`)
- payload body size marker (`u32`) + payload bytes

Schema ID is the object type identity (save vs item vs room vs network), while schema version is that schema's evolution version.

## Determinism rules for containers

- `std::vector` serialization preserves insertion order.
- Ordered containers should already be deterministic.
- `std::unordered_map` is canonicalized by sorting entries by key before serialization.
- `std::unordered_set` is canonicalized by sorting values before serialization.

This prevents platform/run-dependent byte differences from hash bucket iteration order.

## Representative schema foundations

- Save payload envelope + early save model (`SavePayload`).
- Recipe bundle payloads for room/bug/item/material recipe descriptors.
- Item payloads with deterministic identity/roll/provenance fields.
- Room payloads with transform, recipe reference, upgrade, seed, and state hooks.
- Network message envelope with type, version, sequence, and binary body.

## Migration workflow hooks

`MigrationRegistry` supports registration of schema-specific migration functions keyed by:

- schema ID
- from version
- to version

Current statement provides the extension point and error model; later statements can add chained migration graphs and offline migration tooling.

## Debugging and inspection

Canonical truth remains binary. Tests assert round-trips, deterministic repeat serialization, version/schema mismatch rejection, and truncated/corrupt payload rejection.
