#include "engine/core/random.hpp"

namespace render::core {

namespace {
constexpr u64 splitmix64(u64& x) noexcept {
  u64 z = (x += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31U);
}
}  // namespace

Random::Random(Seed seed) noexcept : state_(0), inc_((seed.value << 1U) | 1U) {
  u64 init = seed.value;
  state_ = splitmix64(init);
}

u32 Random::next_u32() noexcept {
  const u64 oldstate = state_;
  state_ = oldstate * 6364136223846793005ULL + inc_;
  const u32 xorshifted = static_cast<u32>(((oldstate >> 18U) ^ oldstate) >> 27U);
  const u32 rot = static_cast<u32>(oldstate >> 59U);
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u64 Random::next_u64() noexcept {
  const u64 hi = next_u32();
  const u64 lo = next_u32();
  return (hi << 32U) | lo;
}

f32 Random::next_f32() noexcept {
  return static_cast<f32>(next_u32()) / static_cast<f32>(0xFFFFFFFFU);
}

i32 Random::range_i32(const i32 min_inclusive, const i32 max_exclusive) noexcept {
  const u32 width = static_cast<u32>(max_exclusive - min_inclusive);
  return min_inclusive + static_cast<i32>(next_u32() % width);
}

f32 Random::range_f32(const f32 min_inclusive, const f32 max_inclusive) noexcept {
  return min_inclusive + (max_inclusive - min_inclusive) * next_f32();
}

Seed Random::split(const std::string_view branch) noexcept {
  const auto seed = derive_seed({state_}, branch);
  state_ = hash_combine(state_, seed.value);
  return seed;
}

Seed derive_seed(const Seed& parent, const std::string_view salt) noexcept {
  return {hash_combine(parent.value, hash_string(salt))};
}

}  // namespace render::core
