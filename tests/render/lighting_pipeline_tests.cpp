#include "engine/render/lighting.hpp"

#include <cassert>
#include <vector>

int main() {
  using namespace render;
  using namespace render::rendering;

  std::vector<DirectionalLightInput> directional = {
    {.direction = {0.0F, -1.0F, 0.0F}, .color = {1.0F, 0.95F, 0.9F}, .intensity = 2.0F, .casts_shadows = true},
    {.direction = {1.0F, -0.5F, 0.0F}, .color = {0.2F, 0.3F, 0.8F}, .intensity = 0.4F, .casts_shadows = false},
    {.direction = {-1.0F, -0.5F, 0.0F}, .color = {0.2F, 0.8F, 0.3F}, .intensity = 0.1F, .casts_shadows = false},
  };

  std::vector<PointLightInput> points;
  for (std::uint32_t i = 0; i < 24; ++i) {
    points.push_back(PointLightInput{
      .position = {static_cast<float>(i) * 1.5F, 0.0F, 0.0F},
      .color = {1.0F, 0.3F, 0.2F},
      .intensity = 2.0F - (static_cast<float>(i) * 0.04F),
      .range = 6.0F,
      .casts_shadows = false,
    });
  }

  std::vector<RenderableLightingInput> objects = {
    {.object_id = 1, .position = {0.0F, 0.0F, 0.0F}, .bounding_radius = 0.5F, .highlighted = true},
    {.object_id = 2, .position = {6.0F, 0.0F, 0.0F}, .bounding_radius = 1.0F, .highlighted = false},
    {.object_id = 3, .position = {20.0F, 0.0F, 0.0F}, .bounding_radius = 1.0F, .highlighted = true},
  };

  LightingSelectionConfig config{};
  config.max_directional_lights = 1;
  config.max_point_lights = 8;
  config.max_lights_per_object = 4;

  FogSettings fog{};
  fog.enabled = true;
  fog.mode = FogMode::Linear;
  fog.near_distance = 2.0F;
  fog.far_distance = 60.0F;

  BloomSettings bloom{};
  bloom.enabled = true;
  bloom.threshold = 0.8F;
  bloom.intensity = 1.25F;

  ShadowSettings shadows{};
  shadows.enabled = true;
  shadows.map_size = 1024;

  const LightingFrameData frame = build_forward_plus_frame_data(
    core::Vec3{0.0F, 0.0F, 0.0F},
    directional,
    points,
    objects,
    config,
    fog,
    bloom,
    shadows,
    OutlineSettings{});

  assert(frame.directional_lights.size() == 1);
  assert(frame.diagnostics.shadowed_directional_lights == 1);
  assert(frame.point_lights.size() == 8);
  assert(frame.per_object_lights.size() == objects.size());
  assert(frame.per_object_lights[0].point_light_count > 0);
  assert(frame.per_object_lights[0].point_light_count <= 4);
  assert(frame.diagnostics.highlighted_objects == 2);
  assert(frame.diagnostics.culled_point_lights == (points.size() - frame.point_lights.size()));

  FogSettings invalid_fog{};
  invalid_fog.enabled = true;
  invalid_fog.mode = FogMode::Linear;
  invalid_fog.near_distance = 10.0F;
  invalid_fog.far_distance = 3.0F;
  assert(!validate_fog_settings(invalid_fog));

  BloomSettings invalid_bloom{};
  invalid_bloom.enabled = true;
  invalid_bloom.downsample_steps = 0;
  assert(!validate_bloom_settings(invalid_bloom));

  ShadowSettings invalid_shadow{};
  invalid_shadow.enabled = true;
  invalid_shadow.map_size = 128;
  assert(!validate_shadow_settings(invalid_shadow));

  return 0;
}
