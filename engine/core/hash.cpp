#include "engine/core/hash.hpp"

namespace render::core {

namespace {
constexpr u64 kFnvOffset = 14695981039346656037ULL;
constexpr u64 kFnvPrime = 1099511628211ULL;
}

u64 hash_bytes(const std::span<const std::byte> bytes) noexcept {
  u64 hash = kFnvOffset;
  for (const std::byte byte : bytes) {
    hash ^= static_cast<u64>(byte);
    hash *= kFnvPrime;
  }
  return hash;
}

u64 hash_string(const std::string_view value) noexcept {
  const auto* data = reinterpret_cast<const std::byte*>(value.data());
  return hash_bytes(std::span<const std::byte>(data, value.size()));
}

u64 hash_combine(const u64 lhs, const u64 rhs) noexcept {
  return lhs ^ (rhs + 0x9e3779b97f4a7c15ULL + (lhs << 6U) + (lhs >> 2U));
}

}  // namespace render::core
