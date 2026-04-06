#pragma once

#include "engine/core/math.hpp"
#include "engine/core/random.hpp"
#include "engine/render/draw_submission.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/shader_library.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace render::rendering::vfx {

enum class EffectType : std::uint8_t {
  Spores = 0,
  Dust,
  Embers,
  ResinDrips,
  GlowPulses,
  MarketAmbience,
};

enum class PrimitiveType : std::uint8_t {
  Particle = 0,
  Drip,
  Pulse,
};

struct EffectHandle {
  std::uint32_t value{0};
  [[nodiscard]] bool valid() const noexcept { return value != 0; }
  friend bool operator==(const EffectHandle&, const EffectHandle&) = default;
};

struct FloatRange {
  float min{0.0F};
  float max{0.0F};
};

struct Vec3Range {
  core::Vec3 min{};
  core::Vec3 max{};
};

struct EffectSpawnRegion {
  core::Vec3 extents{1.0F, 1.0F, 1.0F};
};

struct ParticleStyle {
  FloatRange size{0.03F, 0.08F};
  FloatRange lifetime_seconds{2.0F, 6.0F};
  FloatRange emissive_strength{0.0F, 1.0F};
  Vec3Range initial_velocity{{-0.02F, -0.01F, -0.02F}, {0.02F, 0.04F, 0.02F}};
  Vec3Range drift_velocity{{-0.005F, -0.002F, -0.005F}, {0.005F, 0.002F, 0.005F}};
};

struct ResinDripStyle {
  float drip_interval_seconds{1.8F};
  FloatRange drip_length{0.18F, 0.45F};
  FloatRange drip_fall_speed{0.18F, 0.34F};
  FloatRange fade_seconds{0.25F, 0.45F};
};

struct GlowPulseStyle {
  FloatRange radius{0.3F, 0.9F};
  FloatRange intensity{0.6F, 1.6F};
  FloatRange frequency_hz{0.25F, 1.0F};
};

struct EffectDefinition {
  EffectType type{EffectType::Spores};
  PrimitiveType primitive{PrimitiveType::Particle};
  std::string debug_name{};
  std::uint32_t max_particles{64};
  float spawn_rate_per_second{8.0F};
  bool looping{true};
  bool depth_test{true};
  EffectSpawnRegion spawn_region{};
  ParticleStyle particle{};
  ResinDripStyle resin{};
  GlowPulseStyle pulse{};
  core::Seed seed{1};
};

struct EffectVisibility {
  bool enabled{true};
  std::uint32_t layer_mask{0xFFFFFFFFu};
};

struct EffectDiagnostics {
  std::uint32_t total_active_effects{0};
  std::uint32_t total_active_particles{0};
  std::uint32_t spores{0};
  std::uint32_t dust{0};
  std::uint32_t embers{0};
  std::uint32_t resin_drips{0};
  std::uint32_t glow_pulses{0};
  std::uint32_t market_ambience{0};
  std::uint32_t draw_calls{0};
  std::uint32_t uploaded_instances{0};
};

struct FrameSubmission {
  DrawSubmission draw{};
  std::vector<float> transforms{};
};

struct VfxToggleMask {
  bool spores{true};
  bool dust{true};
  bool embers{true};
  bool resin_drips{true};
  bool glow_pulses{true};
  bool market_ambience{true};

  [[nodiscard]] bool enabled(EffectType type) const noexcept;
};

class VfxSystem {
public:
  VfxSystem(Renderer& renderer, ShaderProgramLibrary& shader_library);

  bool initialize();
  void shutdown();

  [[nodiscard]] EffectHandle spawn_effect(const EffectDefinition& definition, const core::Transform& transform, EffectVisibility visibility = {});
  bool destroy_effect(EffectHandle handle);
  bool set_effect_transform(EffectHandle handle, const core::Transform& transform);
  bool set_effect_enabled(EffectHandle handle, bool enabled);

  void update(float delta_seconds);
  [[nodiscard]] std::vector<FrameSubmission> build_frame_submissions(ViewId view, std::uint32_t layer_mask = 0xFFFFFFFFu);

  void set_toggles(const VfxToggleMask& toggles) { toggles_ = toggles; }
  [[nodiscard]] const VfxToggleMask& toggles() const noexcept { return toggles_; }
  [[nodiscard]] EffectDiagnostics diagnostics() const noexcept { return diagnostics_; }

  [[nodiscard]] static EffectDefinition make_spores_definition(std::string debug_name = "spores");
  [[nodiscard]] static EffectDefinition make_dust_definition(std::string debug_name = "dust");
  [[nodiscard]] static EffectDefinition make_embers_definition(std::string debug_name = "embers");
  [[nodiscard]] static EffectDefinition make_resin_drips_definition(std::string debug_name = "resin_drips");
  [[nodiscard]] static EffectDefinition make_glow_pulses_definition(std::string debug_name = "glow_pulses");
  [[nodiscard]] static std::vector<EffectDefinition> make_market_ambience_preset();

private:
  struct ParticleState {
    core::Vec3 position{};
    core::Vec3 velocity{};
    float age_seconds{0.0F};
    float lifetime_seconds{1.0F};
    float size{0.05F};
    float emissive_strength{0.0F};
    float phase{0.0F};
    bool active{false};
  };

  struct EffectState {
    EffectDefinition definition{};
    EffectVisibility visibility{};
    core::Transform transform{};
    core::Random random{core::Seed{1}};
    std::vector<ParticleState> particles{};
    float spawn_accumulator{0.0F};
    float drip_timer{0.0F};
    bool alive{true};
    std::uint32_t generation{1};
  };

  struct RenderResources {
    MeshBufferHandle quad_mesh{};
    IndexBufferHandle quad_indices{};
    ProgramHandle spores_program{};
    ProgramHandle dust_program{};
    ProgramHandle embers_program{};
    ProgramHandle drips_program{};
    ProgramHandle pulses_program{};
  };

  [[nodiscard]] ProgramHandle program_for_effect_type(EffectType type) const;
  void seed_particles(EffectState& effect);
  void respawn_particle(EffectState& effect, ParticleState& particle, bool stagger_spawn);
  void update_effect(EffectState& effect, float delta_seconds);
  void append_submission(std::vector<FrameSubmission>& submissions, ViewId view, const EffectState& effect, std::span<const ParticleState> particles);
  [[nodiscard]] core::Vec3 random_vec3(core::Random& random, const Vec3Range& range);
  [[nodiscard]] float random_range(core::Random& random, const FloatRange& range);

  Renderer& renderer_;
  ShaderProgramLibrary& shader_library_;
  RenderResources resources_{};
  std::unordered_map<std::uint32_t, EffectState> effects_{};
  std::uint32_t next_effect_handle_{1};
  VfxToggleMask toggles_{};
  EffectDiagnostics diagnostics_{};
};

}  // namespace render::rendering::vfx
