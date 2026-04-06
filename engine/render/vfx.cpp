#include "engine/render/vfx.hpp"

#include <bgfx/bgfx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace render::rendering::vfx {
namespace {

struct PosColorVertex {
  float x;
  float y;
  float z;
  std::uint32_t abgr;
};

constexpr std::uint64_t kVfxBlendState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z
  | BGFX_STATE_DEPTH_TEST_LEQUAL | BGFX_STATE_BLEND_ALPHA;

std::array<float, 16> compose_particle_transform(const core::Vec3 position, const float scale_y, const float scale_xz) {
  core::Transform t{};
  t.translation = position;
  t.scale = {scale_xz, scale_y, scale_xz};
  return t.to_matrix().m;
}

bool is_particle_type(const PrimitiveType primitive) {
  return primitive == PrimitiveType::Particle || primitive == PrimitiveType::Drip;
}

}  // namespace

bool VfxToggleMask::enabled(const EffectType type) const noexcept {
  switch (type) {
    case EffectType::Spores: return spores;
    case EffectType::Dust: return dust;
    case EffectType::Embers: return embers;
    case EffectType::ResinDrips: return resin_drips;
    case EffectType::GlowPulses: return glow_pulses;
    case EffectType::MarketAmbience: return market_ambience;
    default: return true;
  }
}

VfxSystem::VfxSystem(Renderer& renderer, ShaderProgramLibrary& shader_library)
  : renderer_(renderer),
    shader_library_(shader_library) {}

bool VfxSystem::initialize() {
  const std::array<PosColorVertex, 4> vertices = {
    PosColorVertex{-0.5F, 0.0F, 0.0F, 0xffffffff},
    PosColorVertex{0.5F, 0.0F, 0.0F, 0xffffffff},
    PosColorVertex{0.5F, 1.0F, 0.0F, 0xffffffff},
    PosColorVertex{-0.5F, 1.0F, 0.0F, 0xffffffff},
  };
  const std::array<std::uint16_t, 6> indices = {0, 1, 2, 2, 3, 0};

  MeshBufferDescription mesh_desc{};
  mesh_desc.layout.stride = static_cast<std::uint16_t>(sizeof(PosColorVertex));
  mesh_desc.layout.elements = {
    {VertexAttribute::Position, 0, 3, AttributeType::Float, false, false},
    {VertexAttribute::Color0, 12, 4, AttributeType::Uint8, true, false},
  };
  mesh_desc.data = std::as_bytes(std::span{vertices});
  mesh_desc.vertex_count = static_cast<std::uint32_t>(vertices.size());
  mesh_desc.usage = BufferUsage::Immutable;

  IndexBufferDescription index_desc{};
  index_desc.data = std::as_bytes(std::span{indices});
  index_desc.index_count = static_cast<std::uint32_t>(indices.size());

  resources_.quad_mesh = renderer_.create_mesh_buffer(mesh_desc);
  resources_.quad_indices = renderer_.create_index_buffer(index_desc);

  resources_.spores_program = shader_library_.load_program({.category = "vfx", .name = "ambient_sprite", .variant = "spores"});
  resources_.dust_program = shader_library_.load_program({.category = "vfx", .name = "ambient_sprite", .variant = "dust"});
  resources_.embers_program = shader_library_.load_program({.category = "vfx", .name = "ambient_sprite", .variant = "embers"});
  resources_.drips_program = shader_library_.load_program({.category = "vfx", .name = "ambient_sprite", .variant = "resin_drip"});
  resources_.pulses_program = shader_library_.load_program({.category = "vfx", .name = "ambient_sprite", .variant = "glow_pulse"});

  return resources_.quad_mesh.idx != kInvalidHandle && resources_.quad_indices.idx != kInvalidHandle;
}

void VfxSystem::shutdown() {
  for (auto& [_, effect] : effects_) {
    effect.alive = false;
  }
  effects_.clear();
  renderer_.destroy_program(resources_.spores_program);
  renderer_.destroy_program(resources_.dust_program);
  renderer_.destroy_program(resources_.embers_program);
  renderer_.destroy_program(resources_.drips_program);
  renderer_.destroy_program(resources_.pulses_program);
  renderer_.destroy_buffer(resources_.quad_mesh);
  renderer_.destroy_buffer(resources_.quad_indices);
  resources_ = {};
}

EffectHandle VfxSystem::spawn_effect(const EffectDefinition& definition, const core::Transform& transform, const EffectVisibility visibility) {
  EffectState state{};
  state.definition = definition;
  state.visibility = visibility;
  state.transform = transform;
  state.random = core::Random(definition.seed);
  state.particles.resize(definition.max_particles);
  seed_particles(state);

  const std::uint32_t handle = next_effect_handle_++;
  effects_.emplace(handle, std::move(state));
  return EffectHandle{handle};
}

bool VfxSystem::destroy_effect(const EffectHandle handle) {
  return effects_.erase(handle.value) > 0;
}

bool VfxSystem::set_effect_transform(const EffectHandle handle, const core::Transform& transform) {
  auto it = effects_.find(handle.value);
  if (it == effects_.end()) {
    return false;
  }
  it->second.transform = transform;
  return true;
}

bool VfxSystem::set_effect_enabled(const EffectHandle handle, const bool enabled) {
  auto it = effects_.find(handle.value);
  if (it == effects_.end()) {
    return false;
  }
  it->second.visibility.enabled = enabled;
  return true;
}

void VfxSystem::update(const float delta_seconds) {
  for (auto& [_, effect] : effects_) {
    if (!effect.alive || !effect.visibility.enabled || !toggles_.enabled(effect.definition.type)) {
      continue;
    }
    update_effect(effect, delta_seconds);
  }
}

std::vector<FrameSubmission> VfxSystem::build_frame_submissions(const ViewId view, const std::uint32_t layer_mask) {
  diagnostics_ = {};
  std::vector<FrameSubmission> submissions;

  for (const auto& [_, effect] : effects_) {
    if (!effect.alive || !effect.visibility.enabled || (effect.visibility.layer_mask & layer_mask) == 0U
        || !toggles_.enabled(effect.definition.type)) {
      continue;
    }

    std::vector<ParticleState> alive_particles;
    alive_particles.reserve(effect.particles.size());
    for (const auto& particle : effect.particles) {
      if (particle.active) {
        alive_particles.push_back(particle);
      }
    }
    if (alive_particles.empty()) {
      continue;
    }

    append_submission(submissions, view, effect, alive_particles);
    diagnostics_.total_active_effects += 1;
    diagnostics_.total_active_particles += static_cast<std::uint32_t>(alive_particles.size());
    diagnostics_.uploaded_instances += static_cast<std::uint32_t>(alive_particles.size());
    switch (effect.definition.type) {
      case EffectType::Spores: diagnostics_.spores += static_cast<std::uint32_t>(alive_particles.size()); break;
      case EffectType::Dust: diagnostics_.dust += static_cast<std::uint32_t>(alive_particles.size()); break;
      case EffectType::Embers: diagnostics_.embers += static_cast<std::uint32_t>(alive_particles.size()); break;
      case EffectType::ResinDrips: diagnostics_.resin_drips += static_cast<std::uint32_t>(alive_particles.size()); break;
      case EffectType::GlowPulses: diagnostics_.glow_pulses += static_cast<std::uint32_t>(alive_particles.size()); break;
      case EffectType::MarketAmbience: diagnostics_.market_ambience += static_cast<std::uint32_t>(alive_particles.size()); break;
      default: break;
    }
  }

  diagnostics_.draw_calls = static_cast<std::uint32_t>(submissions.size());
  return submissions;
}

EffectDefinition VfxSystem::make_spores_definition(std::string debug_name) {
  EffectDefinition d{};
  d.type = EffectType::Spores;
  d.primitive = PrimitiveType::Particle;
  d.debug_name = std::move(debug_name);
  d.max_particles = 220;
  d.spawn_rate_per_second = 48.0F;
  d.spawn_region.extents = {3.0F, 2.0F, 3.0F};
  d.particle.size = {0.03F, 0.09F};
  d.particle.lifetime_seconds = {6.0F, 14.0F};
  d.particle.emissive_strength = {0.35F, 0.75F};
  d.particle.initial_velocity = {{-0.015F, -0.01F, -0.015F}, {0.015F, 0.03F, 0.015F}};
  d.particle.drift_velocity = {{-0.01F, -0.002F, -0.01F}, {0.01F, 0.003F, 0.01F}};
  return d;
}

EffectDefinition VfxSystem::make_dust_definition(std::string debug_name) {
  EffectDefinition d = make_spores_definition(std::move(debug_name));
  d.type = EffectType::Dust;
  d.max_particles = 320;
  d.spawn_rate_per_second = 58.0F;
  d.particle.size = {0.015F, 0.04F};
  d.particle.lifetime_seconds = {8.0F, 20.0F};
  d.particle.emissive_strength = {0.02F, 0.08F};
  d.particle.initial_velocity = {{-0.007F, -0.004F, -0.007F}, {0.007F, 0.008F, 0.007F}};
  d.particle.drift_velocity = {{-0.003F, -0.001F, -0.003F}, {0.003F, 0.001F, 0.003F}};
  return d;
}

EffectDefinition VfxSystem::make_embers_definition(std::string debug_name) {
  EffectDefinition d = make_spores_definition(std::move(debug_name));
  d.type = EffectType::Embers;
  d.max_particles = 128;
  d.spawn_rate_per_second = 36.0F;
  d.spawn_region.extents = {1.2F, 0.6F, 1.2F};
  d.particle.size = {0.025F, 0.07F};
  d.particle.lifetime_seconds = {1.2F, 3.8F};
  d.particle.emissive_strength = {0.9F, 1.8F};
  d.particle.initial_velocity = {{-0.03F, 0.12F, -0.03F}, {0.03F, 0.34F, 0.03F}};
  d.particle.drift_velocity = {{-0.025F, 0.01F, -0.025F}, {0.025F, 0.08F, 0.025F}};
  return d;
}

EffectDefinition VfxSystem::make_resin_drips_definition(std::string debug_name) {
  EffectDefinition d{};
  d.type = EffectType::ResinDrips;
  d.primitive = PrimitiveType::Drip;
  d.debug_name = std::move(debug_name);
  d.max_particles = 48;
  d.spawn_rate_per_second = 1.0F;
  d.spawn_region.extents = {1.3F, 0.2F, 1.3F};
  d.particle.size = {0.03F, 0.08F};
  d.particle.lifetime_seconds = {1.2F, 3.2F};
  d.particle.emissive_strength = {0.12F, 0.25F};
  d.resin.drip_interval_seconds = 0.75F;
  d.resin.drip_length = {0.24F, 0.62F};
  d.resin.drip_fall_speed = {0.1F, 0.18F};
  return d;
}

EffectDefinition VfxSystem::make_glow_pulses_definition(std::string debug_name) {
  EffectDefinition d{};
  d.type = EffectType::GlowPulses;
  d.primitive = PrimitiveType::Pulse;
  d.debug_name = std::move(debug_name);
  d.max_particles = 16;
  d.spawn_rate_per_second = 0.0F;
  d.spawn_region.extents = {0.8F, 0.2F, 0.8F};
  d.particle.size = {0.2F, 0.6F};
  d.particle.lifetime_seconds = {100000.0F, 100000.0F};
  d.pulse.radius = {0.2F, 0.9F};
  d.pulse.intensity = {0.6F, 1.8F};
  d.pulse.frequency_hz = {0.2F, 1.0F};
  return d;
}

std::vector<EffectDefinition> VfxSystem::make_market_ambience_preset() {
  auto dust = make_dust_definition("market_dust");
  dust.type = EffectType::MarketAmbience;
  dust.max_particles = 260;
  dust.spawn_region.extents = {5.0F, 1.8F, 5.0F};

  auto embers = make_embers_definition("market_lamp_specks");
  embers.type = EffectType::MarketAmbience;
  embers.max_particles = 48;
  embers.spawn_region.extents = {4.0F, 1.3F, 4.0F};

  auto pulses = make_glow_pulses_definition("market_sign_pulses");
  pulses.type = EffectType::MarketAmbience;
  pulses.max_particles = 8;
  pulses.spawn_region.extents = {3.2F, 0.6F, 3.2F};

  return {dust, embers, pulses};
}

ProgramHandle VfxSystem::program_for_effect_type(const EffectType type) const {
  switch (type) {
    case EffectType::Spores: return resources_.spores_program;
    case EffectType::Dust: return resources_.dust_program;
    case EffectType::Embers: return resources_.embers_program;
    case EffectType::ResinDrips: return resources_.drips_program;
    case EffectType::GlowPulses: return resources_.pulses_program;
    case EffectType::MarketAmbience: return resources_.dust_program;
    default: return resources_.dust_program;
  }
}

void VfxSystem::seed_particles(EffectState& effect) {
  const bool stagger = is_particle_type(effect.definition.primitive);
  for (auto& particle : effect.particles) {
    if (effect.definition.primitive == PrimitiveType::Pulse) {
      respawn_particle(effect, particle, false);
      particle.active = true;
      particle.age_seconds = effect.random.range_f32(0.0F, 5.0F);
      continue;
    }
    respawn_particle(effect, particle, stagger);
  }
}

void VfxSystem::respawn_particle(EffectState& effect, ParticleState& particle, const bool stagger_spawn) {
  const auto& def = effect.definition;
  particle.position = random_vec3(effect.random, {
    .min = {-def.spawn_region.extents.x, -def.spawn_region.extents.y, -def.spawn_region.extents.z},
    .max = {def.spawn_region.extents.x, def.spawn_region.extents.y, def.spawn_region.extents.z},
  });
  particle.velocity = random_vec3(effect.random, def.particle.initial_velocity);
  particle.lifetime_seconds = std::max(0.05F, random_range(effect.random, def.particle.lifetime_seconds));
  particle.age_seconds = stagger_spawn ? effect.random.range_f32(0.0F, particle.lifetime_seconds) : 0.0F;
  particle.size = std::max(0.005F, random_range(effect.random, def.particle.size));
  particle.emissive_strength = random_range(effect.random, def.particle.emissive_strength);
  particle.phase = effect.random.range_f32(0.0F, core::kTwoPi);
  particle.active = def.spawn_rate_per_second > 0.0F || def.primitive == PrimitiveType::Pulse;
}

void VfxSystem::update_effect(EffectState& effect, const float delta_seconds) {
  const auto& def = effect.definition;

  effect.spawn_accumulator += def.spawn_rate_per_second * delta_seconds;
  if (def.primitive == PrimitiveType::Drip) {
    effect.drip_timer += delta_seconds;
  }

  for (auto& particle : effect.particles) {
    if (!particle.active) {
      if (def.looping && effect.spawn_accumulator >= 1.0F) {
        effect.spawn_accumulator -= 1.0F;
        respawn_particle(effect, particle, false);
      }
      continue;
    }

    particle.age_seconds += delta_seconds;
    const float normalized_age = particle.age_seconds / particle.lifetime_seconds;

    if (def.primitive == PrimitiveType::Particle) {
      particle.velocity = particle.velocity + random_vec3(effect.random, def.particle.drift_velocity) * delta_seconds;
      particle.position = particle.position + particle.velocity * delta_seconds;
    } else if (def.primitive == PrimitiveType::Drip) {
      const float speed = random_range(effect.random, def.resin.drip_fall_speed);
      particle.position.y -= speed * delta_seconds;
      particle.size = random_range(effect.random, def.resin.drip_length);
    } else if (def.primitive == PrimitiveType::Pulse) {
      particle.phase += delta_seconds * random_range(effect.random, def.pulse.frequency_hz) * core::kTwoPi;
      const float wave = (std::sin(particle.phase) * 0.5F) + 0.5F;
      particle.size = core::lerp(def.pulse.radius.min, def.pulse.radius.max, wave);
      particle.emissive_strength = core::lerp(def.pulse.intensity.min, def.pulse.intensity.max, wave);
    }

    if (normalized_age >= 1.0F) {
      particle.active = false;
      if (def.looping && (def.primitive != PrimitiveType::Drip || effect.drip_timer >= def.resin.drip_interval_seconds)) {
        effect.drip_timer = 0.0F;
        respawn_particle(effect, particle, false);
      }
    }
  }
}

void VfxSystem::append_submission(
  std::vector<FrameSubmission>& submissions,
  const ViewId view,
  const EffectState& effect,
  const std::span<const ParticleState> particles) {
  FrameSubmission submission{};
  submission.draw.view = view;
  submission.draw.mesh.vertex_buffer = resources_.quad_mesh;
  submission.draw.mesh.index_buffer = resources_.quad_indices;
  submission.draw.mesh.index_count = 6;
  submission.draw.material.program = program_for_effect_type(effect.definition.type);
  submission.draw.draw_state.state_flags = effect.definition.depth_test ? kVfxBlendState : (kVfxBlendState | BGFX_STATE_DEPTH_TEST_ALWAYS);
  submission.transforms.reserve(particles.size() * 16U);

  for (const auto& particle : particles) {
    const core::Vec3 world_position = effect.transform.translation + particle.position;
    float scale_y = particle.size;
    float scale_xz = particle.size;
    if (effect.definition.primitive == PrimitiveType::Drip) {
      scale_y = std::max(scale_y, particle.size * 2.4F);
      scale_xz *= 0.55F;
    }
    const auto transform = compose_particle_transform(world_position, scale_y, scale_xz);
    submission.transforms.insert(submission.transforms.end(), transform.begin(), transform.end());
  }

  submissions.push_back(std::move(submission));
}

core::Vec3 VfxSystem::random_vec3(core::Random& random, const Vec3Range& range) {
  return {
    random.range_f32(range.min.x, range.max.x),
    random.range_f32(range.min.y, range.max.y),
    random.range_f32(range.min.z, range.max.z),
  };
}

float VfxSystem::random_range(core::Random& random, const FloatRange& range) {
  return random.range_f32(range.min, range.max);
}

}  // namespace render::rendering::vfx
