#pragma once

#include "engine/core/math.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace render::rendering {

constexpr std::uint32_t kMaxDirectionalLights = 2;
constexpr std::uint32_t kMaxPointLights = 128;
constexpr std::uint32_t kMaxLightsPerObject = 8;
constexpr std::uint32_t kInvalidLightIndex = 0xFFFFFFFFu;

enum class FogMode : std::uint8_t {
  Disabled = 0,
  Exponential,
  ExponentialSquared,
  Linear,
};

struct FogSettings {
  bool enabled{false};
  FogMode mode{FogMode::ExponentialSquared};
  core::Vec3 color{0.07F, 0.08F, 0.10F};
  float density{0.025F};
  float near_distance{4.0F};
  float far_distance{40.0F};
};

struct BloomSettings {
  bool enabled{true};
  float threshold{1.0F};
  float intensity{0.75F};
  std::uint8_t downsample_steps{4};
};

struct ShadowSettings {
  bool enabled{true};
  std::uint16_t map_size{2048};
  float depth_bias{0.0015F};
  float normal_bias{0.015F};
  float filter_radius{1.5F};
};

struct OutlineSettings {
  bool enabled{true};
  core::Vec3 color{1.0F, 0.92F, 0.35F};
  float thickness_px{2.5F};
  float pulse_hz{0.0F};
};

struct DirectionalLightInput {
  core::Vec3 direction{0.0F, -1.0F, 0.0F};
  core::Vec3 color{1.0F, 1.0F, 1.0F};
  float intensity{1.0F};
  bool casts_shadows{false};
};

struct PointLightInput {
  core::Vec3 position{};
  core::Vec3 color{1.0F, 1.0F, 1.0F};
  float intensity{1.0F};
  float range{8.0F};
  bool casts_shadows{false};
};

struct RenderableLightingInput {
  std::uint32_t object_id{0};
  core::Vec3 position{};
  float bounding_radius{1.0F};
  bool highlighted{false};
};

struct DirectionalLightGpu {
  std::array<float, 4> direction_intensity{};  // xyz + intensity
  std::array<float, 4> color_shadow{};         // rgb + shadow-enabled flag
};

struct PointLightGpu {
  std::array<float, 4> position_range{};     // xyz + range
  std::array<float, 4> color_intensity{};    // rgb + intensity
  std::array<float, 4> metadata{};           // x:invRange^2 y:shadow z/w:reserved
};

struct ObjectLightList {
  std::uint32_t object_id{0};
  std::array<std::uint32_t, kMaxLightsPerObject> point_light_indices{};
  std::uint32_t point_light_count{0};
};

struct LightingDiagnostics {
  std::uint32_t input_directional_lights{0};
  std::uint32_t input_point_lights{0};
  std::uint32_t selected_directional_lights{0};
  std::uint32_t selected_point_lights{0};
  std::uint32_t culled_point_lights{0};
  std::uint32_t highlighted_objects{0};
  std::uint32_t shadowed_directional_lights{0};
};

struct LightingFrameData {
  std::vector<DirectionalLightGpu> directional_lights{};
  std::vector<PointLightGpu> point_lights{};
  std::vector<ObjectLightList> per_object_lights{};
  FogSettings fog{};
  BloomSettings bloom{};
  ShadowSettings shadows{};
  OutlineSettings outlines{};
  LightingDiagnostics diagnostics{};
};

struct LightingSelectionConfig {
  std::uint32_t max_directional_lights{kMaxDirectionalLights};
  std::uint32_t max_point_lights{kMaxPointLights};
  std::uint32_t max_lights_per_object{kMaxLightsPerObject};
};

[[nodiscard]] bool validate_fog_settings(const FogSettings& settings);
[[nodiscard]] bool validate_bloom_settings(const BloomSettings& settings);
[[nodiscard]] bool validate_shadow_settings(const ShadowSettings& settings);

[[nodiscard]] LightingFrameData build_forward_plus_frame_data(
  const core::Vec3& camera_position,
  std::span<const DirectionalLightInput> directional_lights,
  std::span<const PointLightInput> point_lights,
  std::span<const RenderableLightingInput> renderables,
  const LightingSelectionConfig& config,
  FogSettings fog,
  BloomSettings bloom,
  ShadowSettings shadows,
  OutlineSettings outlines);

}  // namespace render::rendering
