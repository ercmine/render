#include "engine/procgen/procgen.hpp"

#include <cassert>
#include <cmath>

int main() {
  using namespace render::procgen;

  const auto box = make_box({1.0F, 2.0F, 3.0F}, 1);
  const auto box_valid = validate_proc_mesh(box);
  assert(box_valid.valid);
  assert(box.vertex_count > 0);
  assert(box.index_count > 0);

  render::core::Seed seed = render::core::Seed::from_string("procgen-seed");
  SegmentDescriptor seg{};
  seg.mesh = make_cylinder(0.2F, 1.0F, 12, 1, true);
  seg.local_from_prev.translation = {0.0F, 0.65F, 0.0F};
  seg.yaw_jitter_radians = 0.25F;
  seg.scale_jitter = 0.1F;
  const std::array<SegmentDescriptor, 3> chain{seg, seg, seg};
  const auto assembled_a = assemble_segments(chain, {.seed = seed});
  const auto assembled_b = assemble_segments(chain, {.seed = seed});
  assert(assembled_a.vertex_data == assembled_b.vertex_data);
  assert(assembled_a.index_data == assembled_b.index_data);

  SplinePath path{};
  path.control_points = {{-1.0F, 0.0F, 0.0F}, {-0.3F, 0.5F, 0.4F}, {0.4F, -0.2F, 0.8F}, {1.0F, 0.1F, 1.2F}};
  std::array<render::core::Vec2, 8> profile{};
  for (std::size_t i = 0; i < profile.size(); ++i) {
    const float a = static_cast<float>(i) / static_cast<float>(profile.size()) * render::core::kTwoPi;
    profile[i] = {std::cos(a) * 0.2F, std::sin(a) * 0.2F};
  }
  const auto extruded = extrude_profile_along_path(profile, path, {.path_samples = 24, .cap_ends = true, .closed_profile = true});
  assert(validate_proc_mesh(extruded).valid);

  std::array<render::core::Vec2, 4> lathe_profile_data{{{0.0F, -0.6F}, {0.35F, -0.4F}, {0.25F, 0.25F}, {0.1F, 0.7F}}};
  const auto lathed = lathe_profile(lathe_profile_data, {.radial_segments = 24, .angle_radians = render::core::kTwoPi});
  assert(validate_proc_mesh(lathed).valid);

  auto blob = sdf_smooth_union(sdf_sphere({0.0F, 0.0F, 0.0F}, 0.7F), sdf_sphere({0.45F, 0.1F, 0.0F}, 0.5F), 0.25F);
  blob = sdf_subtract(blob, sdf_box({0.0F, -0.5F, 0.0F}, {0.15F, 0.15F, 0.15F}));
  ScalarField field(24, 24, 24, 0.08F, {-1.0F, -1.0F, -1.0F});
  rasterize_sdf_to_field(blob, field);
  const auto iso = extract_isosurface(field, 0.0F);
  const auto iso_valid = validate_proc_mesh(iso);
  assert(iso_valid.valid);
  assert(iso.index_count > 0);

  const auto cleaned = remove_degenerate_triangles(recalculate_normals(merge_meshes(std::array{box, lathed, extruded, assembled_a, iso})));
  const auto stats = mesh_stats(cleaned);
  assert(stats.vertex_count > 0);
  assert(stats.index_count > 0);
  assert(std::isfinite(stats.bounds.min.x));

  return 0;
}
