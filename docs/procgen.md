# Procedural Mesh Toolkit (Statement 17)

## Overview

`engine/procgen` now provides the engine-owned procedural geometry foundation used by runtime generation and test fixtures. It is intentionally renderer-backend agnostic, deterministic when seeded, and emits `render::rendering::CpuMeshData` so generated meshes can flow directly into Statement 12 upload/submission paths.

## Module organization

- `engine/procgen/procgen.hpp`
  - Public engine API for deterministic mesh generation and helpers.
  - Primitive generation APIs (`make_box`, `make_uv_sphere`, `make_capsule`, ...).
  - Spline/path sampling and extrusion APIs.
  - Lathe/revolve API.
  - Segment assembly API with explicit seed.
  - Scalar field / SDF authoring and composition APIs.
  - Surface extraction API from sampled fields.
  - Mesh post-processing, validation, and stats APIs.
- `engine/procgen/procgen.cpp`
  - Concrete topology generation implementations and shared mesh utility logic.

## Determinism and seed usage

- Segment assembly consumes `render::core::Seed` and uses `render::core::Random` splits per segment.
- Variation (yaw and scale jitter) is explicit per segment descriptor.
- No hidden global RNG is used.
- Same input recipe + same seed => byte-identical generated CPU mesh buffers in tests.
- Floating-point caveat: exact results assume identical compiler/platform floating-point behavior; topology/ordering is deterministic in-engine.

## Generation modes and conventions

### Primitives

Implemented primitives:
- quad
- plane
- box
- UV sphere
- cylinder
- cone
- torus
- capsule

Conventions:
- Counter-clockwise winding.
- Per-vertex normals and UVs generated.
- Tangent slot is populated with a stable orthonormal basis hook.
- Segment counts are clamped to safe minimums to avoid degenerate topology.

### Spline/path and extrusion

- `SplinePath` stores control points and open/closed state.
- Catmull-Rom interpolation is used for sampling.
- Per-sample frame data (tangent/normal/binormal) is derived for extrusion.
- `extrude_profile_along_path` sweeps a 2D profile through the sampled path.
- Supports closed profiles and optional caps.

### Lathe/revolve

- `lathe_profile` revolves a 2D radius-height profile around the Y axis.
- Supports full revolve and partial revolve (`angle_radians`).
- Optional start/end caps for partial lathes.

### Segment assembly

- `assemble_segments` merges reusable segment meshes using local attachment transforms.
- Optional deterministic yaw/scale jitter per segment.
- Useful for chained bug limbs, organic supports, tails, and decorative modular props.

### Volumes, SDF, and extraction

- `ScalarField` provides grid-sampled scalar storage, write, query, and world-space mapping.
- SDF primitives: sphere, box, capsule, cylinder, plane.
- SDF composition: union, subtraction, intersection, smooth union, translate.
- `rasterize_sdf_to_field` samples an arbitrary SDF into a scalar grid.
- `extract_isosurface` uses marching tetrahedra to produce a local mesh from the scalar field.

## Surface data and post processing

The toolkit includes practical mesh helpers:
- `merge_meshes`
- `transform_mesh`
- `recalculate_normals`
- `remove_degenerate_triangles`
- `mesh_stats` / bounds
- `validate_proc_mesh` (layout, index range, finite data, degenerate counts)

## Renderer integration

Toolkit output is `CpuMeshData` with procgen-owned vertex layout. The shell now uploads generated meshes via normal renderer buffer creation paths and renders:
- primitive mesh
- spline extrusion mesh
- lathed mesh
- segmented assembly mesh
- SDF/volume extracted mesh

No bgfx/back-end detail is referenced from generation code.

## Scope and deferred items

Implemented now:
- deterministic local mesh generation foundation
- practical primitive/extrude/lathe/segment/SDF/volume pipeline
- mesh utility + validation support

Deferred for future statements:
- authoring/editor UX beyond runtime previews
- higher-order remeshing/subdivision pipelines
- very large terrain/world extraction systems
- advanced UV unwrapping strategies for arbitrary extracted surfaces
