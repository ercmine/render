#pragma once

#include "engine/core/hash.hpp"

#include <string_view>

namespace render::core {

struct Seed {
  u64 value{0};

  [[nodiscard]] static Seed from_string(const std::string_view key) noexcept { return {hash_string(key)}; }
};

class Random {
public:
  explicit Random(Seed seed) noexcept;

  [[nodiscard]] u32 next_u32() noexcept;
  [[nodiscard]] u64 next_u64() noexcept;
  [[nodiscard]] f32 next_f32() noexcept;

  [[nodiscard]] i32 range_i32(i32 min_inclusive, i32 max_exclusive) noexcept;
  [[nodiscard]] f32 range_f32(f32 min_inclusive, f32 max_inclusive) noexcept;

  [[nodiscard]] Seed split(const std::string_view branch) noexcept;

private:
  u64 state_;
  u64 inc_;
};

Seed derive_seed(const Seed& parent, const std::string_view salt) noexcept;

}  // namespace render::core
