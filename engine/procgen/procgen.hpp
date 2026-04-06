#pragma once

#include "engine/core/math.hpp"
#include "engine/core/random.hpp"
#include "engine/core/transform.hpp"
#include "engine/render/buffer_types.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace render::procgen {

struct ProcVertex {
  core::Vec3 position{};
  core::Vec3 normal{};
  core::Vec4 tangent{1.0F, 0.0F, 0.0F, 1.0F};
  core::Vec2 uv{};
};

struct Bounds {
  core::Vec3 min{};
  core::Vec3 max{};
};

[[nodiscard]] rendering::VertexLayoutDescription proc_vertex_layout();
[[nodiscard]] rendering::CpuMeshData make_cpu_mesh(std::span<const ProcVertex> vertices, std::span<const std::uint32_t> indices);
[[nodiscard]] std::vector<ProcVertex> unpack_vertices(const rendering::CpuMeshData& mesh);
[[nodiscard]] std::vector<std::uint32_t> unpack_indices_u32(const rendering::CpuMeshData& mesh);

struct MeshValidationReport {
  bool valid{false};
  std::string reason{};
  std::uint32_t degenerate_triangles{0};
};

[[nodiscard]] MeshValidationReport validate_proc_mesh(const rendering::CpuMeshData& mesh);
[[nodiscard]] Bounds compute_bounds(std::span<const ProcVertex> vertices);

struct PrimitiveOptions {
  std::uint32_t radial_segments{16};
  std::uint32_t height_segments{1};
  std::uint32_t width_segments{1};
  std::uint32_t depth_segments{1};
};

[[nodiscard]] rendering::CpuMeshData make_quad(float width, float height);
[[nodiscard]] rendering::CpuMeshData make_plane(float width, float depth, std::uint32_t width_segments, std::uint32_t depth_segments);
[[nodiscard]] rendering::CpuMeshData make_box(const core::Vec3& size, std::uint32_t segments = 1);
[[nodiscard]] rendering::CpuMeshData make_uv_sphere(float radius, std::uint32_t lat_segments, std::uint32_t lon_segments);
[[nodiscard]] rendering::CpuMeshData make_cylinder(float radius, float height, std::uint32_t radial_segments, std::uint32_t height_segments, bool capped);
[[nodiscard]] rendering::CpuMeshData make_cone(float radius, float height, std::uint32_t radial_segments, bool capped);
[[nodiscard]] rendering::CpuMeshData make_torus(float major_radius, float minor_radius, std::uint32_t major_segments, std::uint32_t minor_segments);
[[nodiscard]] rendering::CpuMeshData make_capsule(float radius, float height, std::uint32_t radial_segments, std::uint32_t hemi_segments);

struct PathPoint {
  core::Vec3 position{};
  core::Vec3 tangent{};
  core::Vec3 normal{};
  core::Vec3 binormal{};
  float u{0.0F};
};

struct SplinePath {
  std::vector<core::Vec3> control_points{};
  bool closed{false};
};

[[nodiscard]] PathPoint sample_path(const SplinePath& path, float t);
[[nodiscard]] std::vector<PathPoint> sample_path_uniform(const SplinePath& path, std::uint32_t samples);

struct ExtrudeOptions {
  std::uint32_t path_samples{16};
  bool cap_ends{true};
  bool closed_profile{true};
};

[[nodiscard]] rendering::CpuMeshData extrude_profile_along_path(std::span<const core::Vec2> profile, const SplinePath& path, const ExtrudeOptions& options);

struct LatheOptions {
  std::uint32_t radial_segments{24};
  float angle_radians{core::kTwoPi};
  bool cap_start{false};
  bool cap_end{false};
};

[[nodiscard]] rendering::CpuMeshData lathe_profile(std::span<const core::Vec2> profile_radius_height, const LatheOptions& options);

struct SegmentDescriptor {
  rendering::CpuMeshData mesh{};
  core::Transform local_from_prev{};
  float yaw_jitter_radians{0.0F};
  float scale_jitter{0.0F};
};

struct SegmentAssemblyOptions {
  core::Seed seed{};
};

[[nodiscard]] rendering::CpuMeshData assemble_segments(std::span<const SegmentDescriptor> segments, const SegmentAssemblyOptions& options);

class ScalarField {
public:
  ScalarField(std::uint32_t nx, std::uint32_t ny, std::uint32_t nz, float cell_size, const core::Vec3& origin);

  [[nodiscard]] std::uint32_t nx() const noexcept { return nx_; }
  [[nodiscard]] std::uint32_t ny() const noexcept { return ny_; }
  [[nodiscard]] std::uint32_t nz() const noexcept { return nz_; }
  [[nodiscard]] float cell_size() const noexcept { return cell_size_; }
  [[nodiscard]] const core::Vec3& origin() const noexcept { return origin_; }

  [[nodiscard]] float at(std::uint32_t x, std::uint32_t y, std::uint32_t z) const;
  void set(std::uint32_t x, std::uint32_t y, std::uint32_t z, float value);
  void fill(float value);

  [[nodiscard]] core::Vec3 cell_position(std::uint32_t x, std::uint32_t y, std::uint32_t z) const;

private:
  [[nodiscard]] std::size_t index(std::uint32_t x, std::uint32_t y, std::uint32_t z) const;

  std::uint32_t nx_{0};
  std::uint32_t ny_{0};
  std::uint32_t nz_{0};
  float cell_size_{1.0F};
  core::Vec3 origin_{};
  std::vector<float> data_{};
};

using SignedDistanceFn = std::function<float(const core::Vec3&)>;

[[nodiscard]] SignedDistanceFn sdf_sphere(core::Vec3 center, float radius);
[[nodiscard]] SignedDistanceFn sdf_box(core::Vec3 center, core::Vec3 half_extents);
[[nodiscard]] SignedDistanceFn sdf_capsule(core::Vec3 a, core::Vec3 b, float radius);
[[nodiscard]] SignedDistanceFn sdf_cylinder(core::Vec3 center, float radius, float half_height);
[[nodiscard]] SignedDistanceFn sdf_plane(core::Vec3 normal, float offset);
[[nodiscard]] SignedDistanceFn sdf_translate(SignedDistanceFn field, core::Vec3 offset);
[[nodiscard]] SignedDistanceFn sdf_union(SignedDistanceFn a, SignedDistanceFn b);
[[nodiscard]] SignedDistanceFn sdf_subtract(SignedDistanceFn a, SignedDistanceFn b);
[[nodiscard]] SignedDistanceFn sdf_intersect(SignedDistanceFn a, SignedDistanceFn b);
[[nodiscard]] SignedDistanceFn sdf_smooth_union(SignedDistanceFn a, SignedDistanceFn b, float k);

void rasterize_sdf_to_field(const SignedDistanceFn& sdf, ScalarField& field);
[[nodiscard]] rendering::CpuMeshData extract_isosurface(const ScalarField& field, float iso_value);

[[nodiscard]] rendering::CpuMeshData merge_meshes(std::span<const rendering::CpuMeshData> meshes);
[[nodiscard]] rendering::CpuMeshData transform_mesh(const rendering::CpuMeshData& mesh, const core::Mat4& transform);
[[nodiscard]] rendering::CpuMeshData recalculate_normals(const rendering::CpuMeshData& mesh);
[[nodiscard]] rendering::CpuMeshData remove_degenerate_triangles(const rendering::CpuMeshData& mesh, float epsilon = 1.0e-6F);

struct MeshStats {
  std::uint32_t vertex_count{0};
  std::uint32_t index_count{0};
  Bounds bounds{};
};

[[nodiscard]] MeshStats mesh_stats(const rendering::CpuMeshData& mesh);

}  // namespace render::procgen
