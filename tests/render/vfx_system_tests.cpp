#include "engine/render/vfx.hpp"

#include <cassert>

int main() {
  using namespace render::rendering::vfx;

  const auto spores = VfxSystem::make_spores_definition();
  assert(spores.type == EffectType::Spores);
  assert(spores.max_particles >= 128);
  assert(spores.spawn_rate_per_second > 0.0F);

  const auto dust = VfxSystem::make_dust_definition();
  assert(dust.type == EffectType::Dust);
  assert(dust.particle.emissive_strength.max < spores.particle.emissive_strength.max);

  const auto embers = VfxSystem::make_embers_definition();
  assert(embers.type == EffectType::Embers);
  assert(embers.particle.initial_velocity.max.y > dust.particle.initial_velocity.max.y);

  const auto resin = VfxSystem::make_resin_drips_definition();
  assert(resin.type == EffectType::ResinDrips);
  assert(resin.primitive == PrimitiveType::Drip);
  assert(resin.resin.drip_interval_seconds > 0.0F);

  const auto pulse = VfxSystem::make_glow_pulses_definition();
  assert(pulse.type == EffectType::GlowPulses);
  assert(pulse.primitive == PrimitiveType::Pulse);
  assert(pulse.pulse.frequency_hz.max >= pulse.pulse.frequency_hz.min);

  const auto market = VfxSystem::make_market_ambience_preset();
  assert(market.size() >= 3);
  for (const auto& effect : market) {
    assert(effect.type == EffectType::MarketAmbience);
  }

  VfxToggleMask toggles{};
  toggles.embers = false;
  assert(toggles.enabled(EffectType::Spores));
  assert(!toggles.enabled(EffectType::Embers));

  return 0;
}
