#include "engine/procgen/procgen.hpp"

#include "engine/core/transform.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace render::procgen {
namespace {

constexpr float kPi = 3.14159265358979323846F;

core::Vec2 operator+(const core::Vec2& a, const core::Vec2& b) { return {a.x + b.x, a.y + b.y}; }
core::Vec2 operator-(const core::Vec2& a, const core::Vec2& b) { return {a.x - b.x, a.y - b.y}; }
core::Vec2 operator*(const core::Vec2& v, float s) { return {v.x * s, v.y * s}; }

float length2(const core::Vec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
core::Vec2 normalize2(const core::Vec2& v) {
  const float len = length2(v);
  if (len <= core::kEpsilon) return {};
  return {v.x / len, v.y / len};
}

core::Vec3 mat_mul_point(const core::Mat4& m, const core::Vec3& p) { return core::transform_point(m, p); }
core::Vec3 mat_mul_dir(const core::Mat4& m, const core::Vec3& v) {
  return {
    m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z,
    m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z,
    m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z,
  };
}

void append_triangle(std::vector<std::uint32_t>& indices, std::uint32_t a, std::uint32_t b, std::uint32_t c) {
  indices.push_back(a);
  indices.push_back(b);
  indices.push_back(c);
}

ProcVertex make_vertex(const core::Vec3& p, const core::Vec3& n, const core::Vec2& uv) {
  const core::Vec3 nn = core::normalize(n);
  core::Vec3 tangent = core::normalize(core::cross({0.0F, 1.0F, 0.0F}, nn));
  if (core::length(tangent) <= core::kEpsilon) tangent = {1.0F, 0.0F, 0.0F};
  return {.position = p, .normal = nn, .tangent = {tangent.x, tangent.y, tangent.z, 1.0F}, .uv = uv};
}

void append_grid_indices(std::vector<std::uint32_t>& indices,
                         std::uint32_t rows,
                         std::uint32_t cols,
                         bool close_rows,
                         bool close_cols,
                         std::uint32_t base_index) {
  const std::uint32_t row_end = close_rows ? rows : rows - 1U;
  const std::uint32_t col_end = close_cols ? cols : cols - 1U;
  for (std::uint32_t r = 0; r < row_end; ++r) {
    const std::uint32_t rn = (r + 1U) % rows;
    for (std::uint32_t c = 0; c < col_end; ++c) {
      const std::uint32_t cn = (c + 1U) % cols;
      const std::uint32_t i00 = base_index + r * cols + c;
      const std::uint32_t i10 = base_index + rn * cols + c;
      const std::uint32_t i11 = base_index + rn * cols + cn;
      const std::uint32_t i01 = base_index + r * cols + cn;
      append_triangle(indices, i00, i10, i11);
      append_triangle(indices, i00, i11, i01);
    }
  }
}

core::Vec3 catmull_rom(const core::Vec3& p0, const core::Vec3& p1, const core::Vec3& p2, const core::Vec3& p3, float t) {
  const float t2 = t * t;
  const float t3 = t2 * t;
  const core::Vec3 c0 = p1 * 2.0F;
  const core::Vec3 c1 = (p2 - p0) * t;
  const core::Vec3 c2 = (p0 * 2.0F - p1 * 5.0F + p2 * 4.0F - p3) * t2;
  const core::Vec3 c3 = ((p1 * 3.0F) - (p2 * 3.0F) + p3 - p0) * t3;
  return (c0 + c1 + c2 + c3) *
         0.5F;
}

core::Vec3 sample_position(const SplinePath& path, float t) {
  if (path.control_points.empty()) return {};
  if (path.control_points.size() == 1U) return path.control_points.front();

  const std::size_t count = path.control_points.size();
  const float span = path.closed ? static_cast<float>(count) : static_cast<float>(count - 1U);
  const float ft = core::clamp(t, 0.0F, 1.0F) * span;
  const int seg = std::min(static_cast<int>(std::floor(ft)), static_cast<int>(span - 1.0F));
  const float local_t = ft - static_cast<float>(seg);

  const auto fetch = [&](int idx) {
    if (path.closed) {
      int wrapped = idx % static_cast<int>(count);
      if (wrapped < 0) wrapped += static_cast<int>(count);
      return path.control_points[static_cast<std::size_t>(wrapped)];
    }
    return path.control_points[static_cast<std::size_t>(core::clamp(idx, 0, static_cast<int>(count) - 1))];
  };

  return catmull_rom(fetch(seg - 1), fetch(seg), fetch(seg + 1), fetch(seg + 2), local_t);
}

float sdf_box_impl(const core::Vec3& p, const core::Vec3& b) {
  const core::Vec3 q{std::fabs(p.x) - b.x, std::fabs(p.y) - b.y, std::fabs(p.z) - b.z};
  const core::Vec3 maxq{std::max(q.x, 0.0F), std::max(q.y, 0.0F), std::max(q.z, 0.0F)};
  const float outside = core::length(maxq);
  const float inside = std::min(std::max(q.x, std::max(q.y, q.z)), 0.0F);
  return outside + inside;
}

}  // namespace

rendering::VertexLayoutDescription proc_vertex_layout() {
  rendering::VertexLayoutDescription layout{};
  layout.stride = static_cast<std::uint16_t>(sizeof(ProcVertex));
  layout.elements = {
    {rendering::VertexAttribute::Position, 0, 3, rendering::AttributeType::Float, false, false},
    {rendering::VertexAttribute::Normal, 12, 3, rendering::AttributeType::Float, false, false},
    {rendering::VertexAttribute::Tangent, 24, 4, rendering::AttributeType::Float, false, false},
    {rendering::VertexAttribute::TexCoord0, 40, 2, rendering::AttributeType::Float, false, false},
  };
  return layout;
}

rendering::CpuMeshData make_cpu_mesh(std::span<const ProcVertex> vertices, std::span<const std::uint32_t> indices) {
  rendering::CpuMeshData mesh{};
  mesh.layout = proc_vertex_layout();
  mesh.vertex_count = static_cast<std::uint32_t>(vertices.size());
  mesh.vertex_data.resize(vertices.size_bytes());
  if (!vertices.empty()) std::memcpy(mesh.vertex_data.data(), vertices.data(), vertices.size_bytes());
  mesh.index_type = rendering::IndexType::Uint32;
  mesh.index_count = static_cast<std::uint32_t>(indices.size());
  mesh.index_data.resize(indices.size_bytes());
  if (!indices.empty()) std::memcpy(mesh.index_data.data(), indices.data(), indices.size_bytes());
  return mesh;
}

std::vector<ProcVertex> unpack_vertices(const rendering::CpuMeshData& mesh) {
  if (mesh.layout.stride != sizeof(ProcVertex)) return {};
  std::vector<ProcVertex> vertices(mesh.vertex_count);
  if (!vertices.empty()) std::memcpy(vertices.data(), mesh.vertex_data.data(), mesh.vertex_data.size());
  return vertices;
}

std::vector<std::uint32_t> unpack_indices_u32(const rendering::CpuMeshData& mesh) {
  std::vector<std::uint32_t> indices(mesh.index_count);
  if (mesh.index_type == rendering::IndexType::Uint32) {
    if (!indices.empty()) std::memcpy(indices.data(), mesh.index_data.data(), mesh.index_data.size());
  } else {
    const auto* src = reinterpret_cast<const std::uint16_t*>(mesh.index_data.data());
    for (std::uint32_t i = 0; i < mesh.index_count; ++i) indices[i] = src[i];
  }
  return indices;
}

MeshValidationReport validate_proc_mesh(const rendering::CpuMeshData& mesh) {
  const auto base = rendering::validate_cpu_mesh_data(mesh);
  if (!base.valid) return {.valid = false, .reason = base.reason};
  if (mesh.layout.stride != sizeof(ProcVertex)) return {.valid = false, .reason = "proc mesh requires ProcVertex layout"};
  const auto vertices = unpack_vertices(mesh);
  const auto indices = unpack_indices_u32(mesh);
  std::uint32_t degenerate = 0;
  for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
    if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size()) {
      return {.valid = false, .reason = "mesh index out of range"};
    }
    const core::Vec3 a = vertices[indices[i]].position;
    const core::Vec3 b = vertices[indices[i + 1]].position;
    const core::Vec3 c = vertices[indices[i + 2]].position;
    const float area2 = core::length(core::cross(b - a, c - a));
    if (!std::isfinite(area2)) return {.valid = false, .reason = "mesh contains NaN/Inf geometry"};
    if (area2 <= 1.0e-8F) ++degenerate;
  }
  return {.valid = true, .reason = {}, .degenerate_triangles = degenerate};
}

Bounds compute_bounds(std::span<const ProcVertex> vertices) {
  Bounds b{};
  if (vertices.empty()) return b;
  b.min = b.max = vertices.front().position;
  for (const auto& v : vertices) {
    b.min.x = std::min(b.min.x, v.position.x);
    b.min.y = std::min(b.min.y, v.position.y);
    b.min.z = std::min(b.min.z, v.position.z);
    b.max.x = std::max(b.max.x, v.position.x);
    b.max.y = std::max(b.max.y, v.position.y);
    b.max.z = std::max(b.max.z, v.position.z);
  }
  return b;
}

rendering::CpuMeshData make_quad(const float width, const float height) {
  const float hx = std::max(width, core::kEpsilon) * 0.5F;
  const float hy = std::max(height, core::kEpsilon) * 0.5F;
  std::vector<ProcVertex> v = {
    make_vertex({-hx, -hy, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}),
    make_vertex({hx, -hy, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}),
    make_vertex({hx, hy, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}),
    make_vertex({-hx, hy, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}),
  };
  const std::vector<std::uint32_t> i = {0, 1, 2, 0, 2, 3};
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData make_plane(const float width, const float depth, const std::uint32_t width_segments, const std::uint32_t depth_segments) {
  const std::uint32_t ws = std::max(1U, width_segments);
  const std::uint32_t ds = std::max(1U, depth_segments);
  const float hw = std::max(width, core::kEpsilon) * 0.5F;
  const float hd = std::max(depth, core::kEpsilon) * 0.5F;
  std::vector<ProcVertex> v;
  std::vector<std::uint32_t> i;
  v.reserve((ws + 1U) * (ds + 1U));
  for (std::uint32_t z = 0; z <= ds; ++z) {
    const float vz = static_cast<float>(z) / static_cast<float>(ds);
    for (std::uint32_t x = 0; x <= ws; ++x) {
      const float ux = static_cast<float>(x) / static_cast<float>(ws);
      v.push_back(make_vertex({core::lerp(-hw, hw, ux), 0.0F, core::lerp(-hd, hd, vz)}, {0.0F, 1.0F, 0.0F}, {ux, vz}));
    }
  }
  append_grid_indices(i, ds + 1U, ws + 1U, false, false, 0);
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData make_box(const core::Vec3& size, const std::uint32_t segments) {
  const float hx = std::max(size.x, core::kEpsilon) * 0.5F;
  const float hy = std::max(size.y, core::kEpsilon) * 0.5F;
  const float hz = std::max(size.z, core::kEpsilon) * 0.5F;
  const auto front = make_plane(size.x, size.y, segments, segments);
  const auto back = transform_mesh(front, core::Mat4::translation({0.0F, 0.0F, -2.0F * hz}) * core::Mat4::rotation(core::Quaternion::from_axis_angle({0, 1, 0}, kPi)) * core::Mat4::translation({0.0F, 0.0F, hz}));
  const auto top = transform_mesh(make_plane(size.x, size.z, segments, segments), core::Mat4::translation({0.0F, hy, 0.0F}));
  const auto bottom = transform_mesh(top, core::Mat4::rotation(core::Quaternion::from_axis_angle({1, 0, 0}, kPi)));
  const auto right = transform_mesh(make_plane(size.z, size.y, segments, segments), core::Mat4::rotation(core::Quaternion::from_axis_angle({0, 1, 0}, kPi * 0.5F)) * core::Mat4::translation({hx, 0.0F, 0.0F}));
  const auto left = transform_mesh(right, core::Mat4::rotation(core::Quaternion::from_axis_angle({0, 1, 0}, kPi)));
  std::array<rendering::CpuMeshData, 6> faces{transform_mesh(front, core::Mat4::translation({0.0F, 0.0F, hz})), back, top, bottom, right, left};
  return merge_meshes(faces);
}

rendering::CpuMeshData make_uv_sphere(const float radius, const std::uint32_t lat_segments, const std::uint32_t lon_segments) {
  const std::uint32_t lat = std::max(3U, lat_segments);
  const std::uint32_t lon = std::max(3U, lon_segments);
  const float r = std::max(radius, core::kEpsilon);
  std::vector<ProcVertex> v;
  std::vector<std::uint32_t> i;
  v.reserve((lat + 1U) * (lon + 1U));
  for (std::uint32_t y = 0; y <= lat; ++y) {
    const float fy = static_cast<float>(y) / static_cast<float>(lat);
    const float theta = fy * kPi;
    const float st = std::sin(theta);
    const float ct = std::cos(theta);
    for (std::uint32_t x = 0; x <= lon; ++x) {
      const float fx = static_cast<float>(x) / static_cast<float>(lon);
      const float phi = fx * core::kTwoPi;
      const core::Vec3 n{std::cos(phi) * st, ct, std::sin(phi) * st};
      v.push_back(make_vertex(n * r, n, {fx, fy}));
    }
  }
  append_grid_indices(i, lat + 1U, lon + 1U, false, false, 0);
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData make_cylinder(const float radius,
                                     const float height,
                                     const std::uint32_t radial_segments,
                                     const std::uint32_t height_segments,
                                     const bool capped) {
  const std::uint32_t rs = std::max(3U, radial_segments);
  const std::uint32_t hs = std::max(1U, height_segments);
  const float r = std::max(radius, core::kEpsilon);
  const float hh = std::max(height, core::kEpsilon) * 0.5F;
  std::vector<ProcVertex> v;
  std::vector<std::uint32_t> i;
  for (std::uint32_t y = 0; y <= hs; ++y) {
    const float fy = static_cast<float>(y) / static_cast<float>(hs);
    const float py = core::lerp(-hh, hh, fy);
    for (std::uint32_t s = 0; s <= rs; ++s) {
      const float u = static_cast<float>(s) / static_cast<float>(rs);
      const float a = u * core::kTwoPi;
      const core::Vec3 n{std::cos(a), 0.0F, std::sin(a)};
      v.push_back(make_vertex({n.x * r, py, n.z * r}, n, {u, fy}));
    }
  }
  append_grid_indices(i, hs + 1U, rs + 1U, false, false, 0);

  if (capped) {
    auto append_cap = [&](float y, float ny, bool flip) {
      const std::uint32_t center = static_cast<std::uint32_t>(v.size());
      v.push_back(make_vertex({0.0F, y, 0.0F}, {0.0F, ny, 0.0F}, {0.5F, 0.5F}));
      for (std::uint32_t s = 0; s <= rs; ++s) {
        const float u = static_cast<float>(s) / static_cast<float>(rs);
        const float a = u * core::kTwoPi;
        const float cx = std::cos(a);
        const float cz = std::sin(a);
        v.push_back(make_vertex({cx * r, y, cz * r}, {0.0F, ny, 0.0F}, {cx * 0.5F + 0.5F, cz * 0.5F + 0.5F}));
      }
      for (std::uint32_t s = 0; s < rs; ++s) {
        const std::uint32_t a0 = center + 1U + s;
        const std::uint32_t a1 = center + 1U + s + 1U;
        if (flip) {
          append_triangle(i, center, a0, a1);
        } else {
          append_triangle(i, center, a1, a0);
        }
      }
    };
    append_cap(-hh, -1.0F, true);
    append_cap(hh, 1.0F, false);
  }
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData make_cone(float radius, float height, std::uint32_t radial_segments, bool capped) {
  const std::uint32_t rs = std::max(3U, radial_segments);
  const float r = std::max(radius, core::kEpsilon);
  const float h = std::max(height, core::kEpsilon);
  std::vector<ProcVertex> v;
  std::vector<std::uint32_t> i;
  const core::Vec3 tip{0.0F, h * 0.5F, 0.0F};
  const float y0 = -h * 0.5F;
  for (std::uint32_t s = 0; s <= rs; ++s) {
    const float u = static_cast<float>(s) / static_cast<float>(rs);
    const float a = u * core::kTwoPi;
    const core::Vec3 p{std::cos(a) * r, y0, std::sin(a) * r};
    const core::Vec3 edge = core::normalize(tip - p);
    const core::Vec3 tan{ -std::sin(a), 0.0F, std::cos(a)};
    const core::Vec3 n = core::normalize(core::cross(tan, edge));
    v.push_back(make_vertex(p, n, {u, 1.0F}));
    v.push_back(make_vertex(tip, n, {u, 0.0F}));
  }
  for (std::uint32_t s = 0; s < rs; ++s) {
    const std::uint32_t b = s * 2U;
    append_triangle(i, b, b + 1U, b + 2U);
    append_triangle(i, b + 1U, b + 3U, b + 2U);
  }
  if (capped) {
    const std::uint32_t center = static_cast<std::uint32_t>(v.size());
    v.push_back(make_vertex({0.0F, y0, 0.0F}, {0.0F, -1.0F, 0.0F}, {0.5F, 0.5F}));
    for (std::uint32_t s = 0; s <= rs; ++s) {
      const float u = static_cast<float>(s) / static_cast<float>(rs);
      const float a = u * core::kTwoPi;
      const float cx = std::cos(a);
      const float cz = std::sin(a);
      v.push_back(make_vertex({cx * r, y0, cz * r}, {0.0F, -1.0F, 0.0F}, {cx * 0.5F + 0.5F, cz * 0.5F + 0.5F}));
    }
    for (std::uint32_t s = 0; s < rs; ++s) append_triangle(i, center, center + 2U + s, center + 1U + s);
  }
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData make_torus(float major_radius, float minor_radius, std::uint32_t major_segments, std::uint32_t minor_segments) {
  const std::uint32_t ms = std::max(3U, major_segments);
  const std::uint32_t ns = std::max(3U, minor_segments);
  const float r0 = std::max(major_radius, core::kEpsilon * 2.0F);
  const float r1 = std::max(minor_radius, core::kEpsilon);
  std::vector<ProcVertex> v;
  std::vector<std::uint32_t> i;
  for (std::uint32_t j = 0; j <= ns; ++j) {
    const float vj = static_cast<float>(j) / static_cast<float>(ns);
    const float b = vj * core::kTwoPi;
    const float cb = std::cos(b);
    const float sb = std::sin(b);
    for (std::uint32_t k = 0; k <= ms; ++k) {
      const float uk = static_cast<float>(k) / static_cast<float>(ms);
      const float a = uk * core::kTwoPi;
      const float ca = std::cos(a);
      const float sa = std::sin(a);
      const core::Vec3 center{ca * r0, 0.0F, sa * r0};
      const core::Vec3 n{ca * cb, sb, sa * cb};
      v.push_back(make_vertex(center + n * r1, n, {uk, vj}));
    }
  }
  append_grid_indices(i, ns + 1U, ms + 1U, false, false, 0);
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData make_capsule(float radius, float height, std::uint32_t radial_segments, std::uint32_t hemi_segments) {
  const auto body = make_cylinder(radius, std::max(height - 2.0F * radius, radius), radial_segments, 1, false);
  const auto sphere = make_uv_sphere(radius, hemi_segments * 2U, radial_segments);
  const auto top = transform_mesh(remove_degenerate_triangles(sphere), core::Mat4::translation({0.0F, std::max(height * 0.5F - radius, 0.0F), 0.0F}));
  const auto bottom = transform_mesh(remove_degenerate_triangles(sphere), core::Mat4::rotation(core::Quaternion::from_axis_angle({1, 0, 0}, kPi)) * core::Mat4::translation({0.0F, -std::max(height * 0.5F - radius, 0.0F), 0.0F}));
  std::array<rendering::CpuMeshData, 3> parts{body, top, bottom};
  return remove_degenerate_triangles(merge_meshes(parts));
}

PathPoint sample_path(const SplinePath& path, float t) {
  PathPoint out{};
  out.position = sample_position(path, t);
  const float dt = 1.0F / 512.0F;
  const core::Vec3 p0 = sample_position(path, std::max(0.0F, t - dt));
  const core::Vec3 p1 = sample_position(path, std::min(1.0F, t + dt));
  out.tangent = core::normalize(p1 - p0);
  core::Vec3 up{0.0F, 1.0F, 0.0F};
  if (std::fabs(core::dot(up, out.tangent)) > 0.9F) up = {1.0F, 0.0F, 0.0F};
  out.binormal = core::normalize(core::cross(out.tangent, up));
  out.normal = core::normalize(core::cross(out.binormal, out.tangent));
  out.u = t;
  return out;
}

std::vector<PathPoint> sample_path_uniform(const SplinePath& path, const std::uint32_t samples) {
  const std::uint32_t count = std::max(2U, samples);
  std::vector<PathPoint> pts;
  pts.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(count - 1U);
    pts.push_back(sample_path(path, t));
  }
  return pts;
}

rendering::CpuMeshData extrude_profile_along_path(std::span<const core::Vec2> profile,
                                                   const SplinePath& path,
                                                   const ExtrudeOptions& options) {
  if (profile.size() < 3 || path.control_points.size() < 2) return {};
  const auto path_points = sample_path_uniform(path, options.path_samples);
  const std::uint32_t rings = static_cast<std::uint32_t>(path_points.size());
  const std::uint32_t profile_count = static_cast<std::uint32_t>(profile.size());
  std::vector<ProcVertex> v;
  std::vector<std::uint32_t> i;
  v.reserve(rings * profile_count);

  for (std::uint32_t r = 0; r < rings; ++r) {
    const auto& frame = path_points[r];
    for (std::uint32_t p = 0; p < profile_count; ++p) {
      const core::Vec2 pp = profile[p];
      const core::Vec3 pos = frame.position + frame.normal * pp.x + frame.binormal * pp.y;
      const core::Vec2 prev = profile[(p + profile_count - 1U) % profile_count];
      const core::Vec2 next = profile[(p + 1U) % profile_count];
      const core::Vec2 edge = normalize2(next - prev);
      const core::Vec3 radial = core::normalize(frame.normal * edge.y - frame.binormal * edge.x);
      v.push_back(make_vertex(pos, radial, {static_cast<float>(p) / static_cast<float>(profile_count), frame.u}));
    }
  }
  append_grid_indices(i, rings, profile_count, false, options.closed_profile, 0);

  if (options.cap_ends) {
    auto append_cap = [&](std::uint32_t ring, bool flip) {
      const std::uint32_t center = static_cast<std::uint32_t>(v.size());
      core::Vec3 sum{};
      for (std::uint32_t p = 0; p < profile_count; ++p) sum = sum + v[ring * profile_count + p].position;
      sum = sum / static_cast<float>(profile_count);
      const core::Vec3 cap_normal = flip ? path_points[ring].tangent * -1.0F : path_points[ring].tangent;
      v.push_back(make_vertex(sum, cap_normal, {0.5F, 0.5F}));
      for (std::uint32_t p = 0; p < profile_count; ++p) {
        const std::uint32_t a = ring * profile_count + p;
        const std::uint32_t b = ring * profile_count + ((p + 1U) % profile_count);
        if (flip) append_triangle(i, center, b, a);
        else append_triangle(i, center, a, b);
      }
    };
    append_cap(0, true);
    append_cap(rings - 1U, false);
  }
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData lathe_profile(std::span<const core::Vec2> profile, const LatheOptions& options) {
  if (profile.size() < 2) return {};
  const std::uint32_t radial = std::max(3U, options.radial_segments);
  const bool closed = std::fabs(options.angle_radians - core::kTwoPi) <= 1.0e-4F;
  std::vector<ProcVertex> v;
  std::vector<std::uint32_t> i;
  const std::uint32_t rings = closed ? radial : (radial + 1U);

  for (std::uint32_t r = 0; r < rings; ++r) {
    const float u = static_cast<float>(r) / static_cast<float>(radial);
    const float a = options.angle_radians * u;
    const float ca = std::cos(a);
    const float sa = std::sin(a);
    for (std::uint32_t p = 0; p < profile.size(); ++p) {
      const float rr = std::max(profile[p].x, 0.0F);
      const float yy = profile[p].y;
      const core::Vec3 pos{ca * rr, yy, sa * rr};
      const core::Vec2 prev = profile[p == 0 ? p : p - 1U];
      const std::size_t next_idx = std::min(profile.size() - 1U, static_cast<std::size_t>(p + 1U));
      const core::Vec2 next = profile[next_idx];
      const core::Vec2 deriv = normalize2(next - prev);
      const core::Vec3 n = core::normalize(core::Vec3{ca * deriv.y, -deriv.x, sa * deriv.y});
      v.push_back(make_vertex(pos, n, {u, static_cast<float>(p) / static_cast<float>(profile.size() - 1U)}));
    }
  }
  append_grid_indices(i, rings, static_cast<std::uint32_t>(profile.size()), closed, false, 0);

  auto add_partial_cap = [&](bool end_cap) {
    const std::uint32_t ring = end_cap ? rings - 1U : 0U;
    const std::uint32_t center = static_cast<std::uint32_t>(v.size());
    core::Vec3 centroid{};
    for (std::uint32_t p = 0; p < profile.size(); ++p) centroid = centroid + v[ring * profile.size() + p].position;
    centroid = centroid / static_cast<float>(profile.size());
    const float sign = end_cap ? 1.0F : -1.0F;
    v.push_back(make_vertex(centroid, {std::sin(options.angle_radians) * sign, 0.0F, -std::cos(options.angle_radians) * sign}, {0.5F, 0.5F}));
    for (std::uint32_t p = 0; p + 1U < profile.size(); ++p) {
      const std::uint32_t a = ring * profile.size() + p;
      const std::uint32_t b = ring * profile.size() + p + 1U;
      if (end_cap) append_triangle(i, center, a, b);
      else append_triangle(i, center, b, a);
    }
  };

  if (!closed && options.cap_start) add_partial_cap(false);
  if (!closed && options.cap_end) add_partial_cap(true);

  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData assemble_segments(std::span<const SegmentDescriptor> segments, const SegmentAssemblyOptions& options) {
  if (segments.empty()) return {};
  core::Random rng(options.seed);
  std::vector<rendering::CpuMeshData> transformed;
  transformed.reserve(segments.size());
  core::Transform running{};
  for (std::size_t s = 0; s < segments.size(); ++s) {
    const SegmentDescriptor& seg = segments[s];
    const core::Seed seg_seed = rng.split("segment");
    core::Random local(seg_seed);

    core::Transform jitter = seg.local_from_prev;
    jitter.rotation = core::Quaternion::from_axis_angle({0.0F, 1.0F, 0.0F}, local.range_f32(-seg.yaw_jitter_radians, seg.yaw_jitter_radians)) *
                      jitter.rotation;
    const float scale = 1.0F + local.range_f32(-seg.scale_jitter, seg.scale_jitter);
    jitter.scale = jitter.scale * scale;

    running = (s == 0U) ? jitter : core::compose(running, jitter);
    transformed.push_back(transform_mesh(seg.mesh, running.to_matrix()));
  }
  return merge_meshes(transformed);
}

ScalarField::ScalarField(const std::uint32_t nx, const std::uint32_t ny, const std::uint32_t nz, const float cell_size, const core::Vec3& origin)
    : nx_(std::max(2U, nx)), ny_(std::max(2U, ny)), nz_(std::max(2U, nz)), cell_size_(std::max(cell_size, core::kEpsilon)), origin_(origin),
      data_(static_cast<std::size_t>(nx_) * ny_ * nz_, 1.0F) {}

std::size_t ScalarField::index(const std::uint32_t x, const std::uint32_t y, const std::uint32_t z) const {
  return static_cast<std::size_t>(z) * nx_ * ny_ + static_cast<std::size_t>(y) * nx_ + x;
}

float ScalarField::at(const std::uint32_t x, const std::uint32_t y, const std::uint32_t z) const {
  return data_[index(std::min(x, nx_ - 1U), std::min(y, ny_ - 1U), std::min(z, nz_ - 1U))];
}

void ScalarField::set(const std::uint32_t x, const std::uint32_t y, const std::uint32_t z, const float value) {
  data_[index(std::min(x, nx_ - 1U), std::min(y, ny_ - 1U), std::min(z, nz_ - 1U))] = value;
}

void ScalarField::fill(const float value) {
  std::fill(data_.begin(), data_.end(), value);
}

core::Vec3 ScalarField::cell_position(const std::uint32_t x, const std::uint32_t y, const std::uint32_t z) const {
  return {origin_.x + static_cast<float>(x) * cell_size_, origin_.y + static_cast<float>(y) * cell_size_,
          origin_.z + static_cast<float>(z) * cell_size_};
}

SignedDistanceFn sdf_sphere(core::Vec3 center, float radius) {
  radius = std::max(radius, core::kEpsilon);
  return [center, radius](const core::Vec3& p) { return core::length(p - center) - radius; };
}

SignedDistanceFn sdf_box(core::Vec3 center, core::Vec3 half_extents) {
  half_extents.x = std::max(half_extents.x, core::kEpsilon);
  half_extents.y = std::max(half_extents.y, core::kEpsilon);
  half_extents.z = std::max(half_extents.z, core::kEpsilon);
  return [center, half_extents](const core::Vec3& p) { return sdf_box_impl(p - center, half_extents); };
}

SignedDistanceFn sdf_capsule(core::Vec3 a, core::Vec3 b, float radius) {
  radius = std::max(radius, core::kEpsilon);
  return [a, b, radius](const core::Vec3& p) {
    const core::Vec3 pa = p - a;
    const core::Vec3 ba = b - a;
    const float h = core::clamp(core::dot(pa, ba) / std::max(core::dot(ba, ba), core::kEpsilon), 0.0F, 1.0F);
    return core::length(pa - ba * h) - radius;
  };
}

SignedDistanceFn sdf_cylinder(core::Vec3 center, float radius, float half_height) {
  radius = std::max(radius, core::kEpsilon);
  half_height = std::max(half_height, core::kEpsilon);
  return [center, radius, half_height](const core::Vec3& p) {
    const core::Vec3 d = p - center;
    const core::Vec2 q{length2({d.x, d.z}) - radius, std::fabs(d.y) - half_height};
    return std::min(std::max(q.x, q.y), 0.0F) + length2({std::max(q.x, 0.0F), std::max(q.y, 0.0F)});
  };
}

SignedDistanceFn sdf_plane(core::Vec3 normal, float offset) {
  normal = core::normalize(normal);
  return [normal, offset](const core::Vec3& p) { return core::dot(normal, p) + offset; };
}

SignedDistanceFn sdf_translate(SignedDistanceFn field, core::Vec3 offset) {
  return [field = std::move(field), offset](const core::Vec3& p) { return field(p - offset); };
}

SignedDistanceFn sdf_union(SignedDistanceFn a, SignedDistanceFn b) {
  return [a = std::move(a), b = std::move(b)](const core::Vec3& p) { return std::min(a(p), b(p)); };
}

SignedDistanceFn sdf_subtract(SignedDistanceFn a, SignedDistanceFn b) {
  return [a = std::move(a), b = std::move(b)](const core::Vec3& p) { return std::max(a(p), -b(p)); };
}

SignedDistanceFn sdf_intersect(SignedDistanceFn a, SignedDistanceFn b) {
  return [a = std::move(a), b = std::move(b)](const core::Vec3& p) { return std::max(a(p), b(p)); };
}

SignedDistanceFn sdf_smooth_union(SignedDistanceFn a, SignedDistanceFn b, float k) {
  k = std::max(k, core::kEpsilon);
  return [a = std::move(a), b = std::move(b), k](const core::Vec3& p) {
    const float da = a(p);
    const float db = b(p);
    const float h = core::clamp(0.5F + 0.5F * (db - da) / k, 0.0F, 1.0F);
    return core::lerp(db, da, h) - k * h * (1.0F - h);
  };
}

void rasterize_sdf_to_field(const SignedDistanceFn& sdf, ScalarField& field) {
  for (std::uint32_t z = 0; z < field.nz(); ++z) {
    for (std::uint32_t y = 0; y < field.ny(); ++y) {
      for (std::uint32_t x = 0; x < field.nx(); ++x) {
        field.set(x, y, z, sdf(field.cell_position(x, y, z)));
      }
    }
  }
}

rendering::CpuMeshData extract_isosurface(const ScalarField& field, const float iso_value) {
  static constexpr std::array<std::array<int, 4>, 6> kTets = {{{0, 5, 1, 6}, {0, 1, 2, 6}, {0, 2, 3, 6}, {0, 3, 7, 6}, {0, 7, 4, 6}, {0, 4, 5, 6}}};
  static constexpr std::array<core::Vec3, 8> kOff = {{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}};

  auto interpolate = [&](const core::Vec3& p0, const core::Vec3& p1, float v0, float v1) {
    const float denom = (v1 - v0);
    const float t = std::fabs(denom) <= core::kEpsilon ? 0.5F : core::clamp((iso_value - v0) / denom, 0.0F, 1.0F);
    return p0 + (p1 - p0) * t;
  };

  std::vector<ProcVertex> verts;
  std::vector<std::uint32_t> indices;

  for (std::uint32_t z = 0; z + 1U < field.nz(); ++z) {
    for (std::uint32_t y = 0; y + 1U < field.ny(); ++y) {
      for (std::uint32_t x = 0; x + 1U < field.nx(); ++x) {
        std::array<core::Vec3, 8> p{};
        std::array<float, 8> v{};
        for (std::size_t c = 0; c < 8; ++c) {
          p[c] = field.cell_position(x + static_cast<std::uint32_t>(kOff[c].x), y + static_cast<std::uint32_t>(kOff[c].y),
                                     z + static_cast<std::uint32_t>(kOff[c].z));
          v[c] = field.at(x + static_cast<std::uint32_t>(kOff[c].x), y + static_cast<std::uint32_t>(kOff[c].y),
                          z + static_cast<std::uint32_t>(kOff[c].z));
        }

        for (const auto& tet : kTets) {
          std::array<int, 4> inside{};
          int inside_count = 0;
          for (int t = 0; t < 4; ++t) {
            if (v[tet[t]] <= iso_value) inside[inside_count++] = t;
          }
          if (inside_count == 0 || inside_count == 4) continue;

          auto point = [&](int a, int b) { return interpolate(p[tet[a]], p[tet[b]], v[tet[a]], v[tet[b]]); };
          auto add_tri = [&](const core::Vec3& a, const core::Vec3& b, const core::Vec3& c) {
            const core::Vec3 n = core::normalize(core::cross(b - a, c - a));
            const std::uint32_t base = static_cast<std::uint32_t>(verts.size());
            verts.push_back(make_vertex(a, n, {0.0F, 0.0F}));
            verts.push_back(make_vertex(b, n, {1.0F, 0.0F}));
            verts.push_back(make_vertex(c, n, {0.0F, 1.0F}));
            append_triangle(indices, base, base + 1U, base + 2U);
          };

          if (inside_count == 1 || inside_count == 3) {
            int i0 = -1;
            std::array<int, 3> o{};
            int oi = 0;
            if (inside_count == 1) {
              i0 = inside[0];
              for (int t = 0; t < 4; ++t) if (t != i0) o[oi++] = t;
              add_tri(point(i0, o[0]), point(i0, o[1]), point(i0, o[2]));
            } else {
              std::array<bool, 4> is_inside{false, false, false, false};
              for (int t = 0; t < 3; ++t) is_inside[inside[t]] = true;
              for (int t = 0; t < 4; ++t) if (!is_inside[t]) i0 = t;
              for (int t = 0; t < 4; ++t) if (t != i0) o[oi++] = t;
              add_tri(point(i0, o[0]), point(i0, o[2]), point(i0, o[1]));
            }
          } else {
            std::array<int, 2> in{};
            std::array<int, 2> out{};
            int oo = 0;
            std::array<bool, 4> is_inside{false, false, false, false};
            for (int t = 0; t < 2; ++t) {
              in[t] = inside[t];
              is_inside[inside[t]] = true;
            }
            for (int t = 0; t < 4; ++t) {
              if (!is_inside[t]) out[oo++] = t;
            }
            const core::Vec3 a = point(in[0], out[0]);
            const core::Vec3 b = point(in[0], out[1]);
            const core::Vec3 c = point(in[1], out[0]);
            const core::Vec3 d = point(in[1], out[1]);
            add_tri(a, b, c);
            add_tri(c, b, d);
          }
        }
      }
    }
  }
  return remove_degenerate_triangles(make_cpu_mesh(verts, indices));
}

rendering::CpuMeshData merge_meshes(std::span<const rendering::CpuMeshData> meshes) {
  std::vector<ProcVertex> out_v;
  std::vector<std::uint32_t> out_i;
  std::uint32_t base = 0;
  for (const auto& mesh : meshes) {
    if (mesh.vertex_count == 0 || mesh.index_count == 0) continue;
    const auto v = unpack_vertices(mesh);
    const auto i = unpack_indices_u32(mesh);
    out_v.insert(out_v.end(), v.begin(), v.end());
    out_i.reserve(out_i.size() + i.size());
    for (const auto idx : i) out_i.push_back(base + idx);
    base += static_cast<std::uint32_t>(v.size());
  }
  return make_cpu_mesh(out_v, out_i);
}

rendering::CpuMeshData transform_mesh(const rendering::CpuMeshData& mesh, const core::Mat4& transform) {
  auto v = unpack_vertices(mesh);
  const auto i = unpack_indices_u32(mesh);
  for (auto& vx : v) {
    vx.position = mat_mul_point(transform, vx.position);
    vx.normal = core::normalize(mat_mul_dir(transform, vx.normal));
    const core::Vec3 t{vx.tangent.x, vx.tangent.y, vx.tangent.z};
    const core::Vec3 nt = core::normalize(mat_mul_dir(transform, t));
    vx.tangent = {nt.x, nt.y, nt.z, vx.tangent.w};
  }
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData recalculate_normals(const rendering::CpuMeshData& mesh) {
  auto v = unpack_vertices(mesh);
  const auto i = unpack_indices_u32(mesh);
  for (auto& vv : v) vv.normal = {0.0F, 0.0F, 0.0F};
  for (std::size_t t = 0; t + 2 < i.size(); t += 3) {
    const auto a = i[t];
    const auto b = i[t + 1];
    const auto c = i[t + 2];
    const core::Vec3 n = core::cross(v[b].position - v[a].position, v[c].position - v[a].position);
    v[a].normal = v[a].normal + n;
    v[b].normal = v[b].normal + n;
    v[c].normal = v[c].normal + n;
  }
  for (auto& vv : v) vv.normal = core::normalize(vv.normal);
  return make_cpu_mesh(v, i);
}

rendering::CpuMeshData remove_degenerate_triangles(const rendering::CpuMeshData& mesh, const float epsilon) {
  const auto v = unpack_vertices(mesh);
  const auto i = unpack_indices_u32(mesh);
  std::vector<std::uint32_t> filtered;
  filtered.reserve(i.size());
  for (std::size_t t = 0; t + 2 < i.size(); t += 3) {
    const core::Vec3 a = v[i[t]].position;
    const core::Vec3 b = v[i[t + 1]].position;
    const core::Vec3 c = v[i[t + 2]].position;
    if (core::length(core::cross(b - a, c - a)) > epsilon) {
      append_triangle(filtered, i[t], i[t + 1], i[t + 2]);
    }
  }
  return make_cpu_mesh(v, filtered);
}

MeshStats mesh_stats(const rendering::CpuMeshData& mesh) {
  const auto vertices = unpack_vertices(mesh);
  return {.vertex_count = mesh.vertex_count, .index_count = mesh.index_count, .bounds = compute_bounds(vertices)};
}

}  // namespace render::procgen
