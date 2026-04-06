#include "engine/render/lighting.hpp"

#include "engine/core/math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace render::rendering {
namespace {

struct RankedPointLight {
  std::uint32_t source_index{0};
  float score{0.0F};
};

float saturate(const float value) {
  return core::clamp(value, 0.0F, 1.0F);
}

float point_light_importance(const PointLightInput& light, const core::Vec3 camera_position) {
  const core::Vec3 to_light = light.position - camera_position;
  const float distance = core::length(to_light);
  const float normalized = distance / std::max(light.range, core::kEpsilon);
  const float attenuation = 1.0F / (1.0F + normalized * normalized);
  return std::max(0.0F, light.intensity) * attenuation;
}

float light_to_object_score(const PointLightGpu& light, const RenderableLightingInput& renderable) {
  const core::Vec3 light_position{light.position_range[0], light.position_range[1], light.position_range[2]};
  const core::Vec3 to_object = renderable.position - light_position;
  const float distance = core::length(to_object);
  const float effective_range = light.position_range[3] + std::max(0.0F, renderable.bounding_radius);
  if (distance > effective_range) {
    return 0.0F;
  }

  const float normalized = distance / std::max(effective_range, core::kEpsilon);
  const float falloff = 1.0F - saturate(normalized * normalized);
  return light.color_intensity[3] * falloff;
}

DirectionalLightGpu pack_directional(const DirectionalLightInput& light) {
  const core::Vec3 normalized = core::normalize(light.direction);
  return DirectionalLightGpu{
    .direction_intensity = {normalized.x, normalized.y, normalized.z, std::max(0.0F, light.intensity)},
    .color_shadow = {
      std::max(0.0F, light.color.x),
      std::max(0.0F, light.color.y),
      std::max(0.0F, light.color.z),
      light.casts_shadows ? 1.0F : 0.0F,
    },
  };
}

PointLightGpu pack_point(const PointLightInput& light) {
  const float clamped_range = std::max(light.range, 0.01F);
  const float inv_range2 = 1.0F / (clamped_range * clamped_range);
  return PointLightGpu{
    .position_range = {light.position.x, light.position.y, light.position.z, clamped_range},
    .color_intensity = {
      std::max(0.0F, light.color.x),
      std::max(0.0F, light.color.y),
      std::max(0.0F, light.color.z),
      std::max(0.0F, light.intensity),
    },
    .metadata = {inv_range2, light.casts_shadows ? 1.0F : 0.0F, 0.0F, 0.0F},
  };
}

}  // namespace

bool validate_fog_settings(const FogSettings& settings) {
  if (!settings.enabled) {
    return true;
  }

  if (settings.mode == FogMode::Linear) {
    return settings.far_distance > settings.near_distance;
  }

  return settings.density >= 0.0F;
}

bool validate_bloom_settings(const BloomSettings& settings) {
  if (!settings.enabled) {
    return true;
  }

  return settings.threshold >= 0.0F && settings.intensity >= 0.0F && settings.downsample_steps > 0;
}

bool validate_shadow_settings(const ShadowSettings& settings) {
  if (!settings.enabled) {
    return true;
  }

  return settings.map_size >= 256U && settings.depth_bias >= 0.0F && settings.normal_bias >= 0.0F && settings.filter_radius >= 0.0F;
}

LightingFrameData build_forward_plus_frame_data(
  const core::Vec3& camera_position,
  const std::span<const DirectionalLightInput> directional_lights,
  const std::span<const PointLightInput> point_lights,
  const std::span<const RenderableLightingInput> renderables,
  const LightingSelectionConfig& config,
  FogSettings fog,
  BloomSettings bloom,
  ShadowSettings shadows,
  OutlineSettings outlines) {
  LightingFrameData result{};
  result.fog = fog;
  result.bloom = bloom;
  result.shadows = shadows;
  result.outlines = outlines;
  result.diagnostics.input_directional_lights = static_cast<std::uint32_t>(directional_lights.size());
  result.diagnostics.input_point_lights = static_cast<std::uint32_t>(point_lights.size());

  const std::uint32_t directional_cap = std::min(config.max_directional_lights, kMaxDirectionalLights);
  for (std::uint32_t i = 0; i < directional_cap && i < directional_lights.size(); ++i) {
    const DirectionalLightInput& source = directional_lights[i];
    if (source.intensity <= 0.0F) {
      continue;
    }
    result.directional_lights.push_back(pack_directional(source));
    if (source.casts_shadows) {
      result.diagnostics.shadowed_directional_lights += 1;
    }
  }

  std::vector<RankedPointLight> ranked;
  ranked.reserve(point_lights.size());
  for (std::uint32_t i = 0; i < point_lights.size(); ++i) {
    const PointLightInput& source = point_lights[i];
    if (source.intensity <= 0.0F || source.range <= 0.0F) {
      continue;
    }
    ranked.push_back({.source_index = i, .score = point_light_importance(source, camera_position)});
  }
  std::sort(ranked.begin(), ranked.end(), [](const RankedPointLight& a, const RankedPointLight& b) {
    if (a.score == b.score) {
      return a.source_index < b.source_index;
    }
    return a.score > b.score;
  });

  const std::uint32_t point_cap = std::min(config.max_point_lights, kMaxPointLights);
  for (std::uint32_t i = 0; i < point_cap && i < ranked.size(); ++i) {
    result.point_lights.push_back(pack_point(point_lights[ranked[i].source_index]));
  }

  result.per_object_lights.reserve(renderables.size());
  for (const RenderableLightingInput& renderable : renderables) {
    ObjectLightList object_list{};
    object_list.object_id = renderable.object_id;
    object_list.point_light_indices.fill(kInvalidLightIndex);

    std::vector<std::pair<std::uint32_t, float>> candidates;
    candidates.reserve(result.point_lights.size());
    for (std::uint32_t i = 0; i < result.point_lights.size(); ++i) {
      const float score = light_to_object_score(result.point_lights[i], renderable);
      if (score > 0.0F) {
        candidates.push_back({i, score});
      }
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
      if (a.second == b.second) {
        return a.first < b.first;
      }
      return a.second > b.second;
    });

    const std::uint32_t per_object_cap = std::min(config.max_lights_per_object, kMaxLightsPerObject);
    const std::uint32_t selected_count = std::min(per_object_cap, static_cast<std::uint32_t>(candidates.size()));
    object_list.point_light_count = selected_count;
    for (std::uint32_t i = 0; i < selected_count; ++i) {
      object_list.point_light_indices[i] = candidates[i].first;
    }

    if (renderable.highlighted) {
      result.diagnostics.highlighted_objects += 1;
    }
    result.per_object_lights.push_back(object_list);
  }

  result.diagnostics.selected_directional_lights = static_cast<std::uint32_t>(result.directional_lights.size());
  result.diagnostics.selected_point_lights = static_cast<std::uint32_t>(result.point_lights.size());
  result.diagnostics.culled_point_lights =
    result.diagnostics.input_point_lights > result.diagnostics.selected_point_lights
    ? (result.diagnostics.input_point_lights - result.diagnostics.selected_point_lights)
    : 0;

  if (!validate_fog_settings(result.fog)) {
    result.fog.enabled = false;
  }
  if (!validate_bloom_settings(result.bloom)) {
    result.bloom.enabled = false;
  }
  if (!validate_shadow_settings(result.shadows)) {
    result.shadows.enabled = false;
  }

  return result;
}

}  // namespace render::rendering
