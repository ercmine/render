#pragma once

#include "engine/core/types.hpp"

#include <span>
#include <string_view>

namespace render::core {

u64 hash_bytes(std::span<const std::byte> bytes) noexcept;
u64 hash_string(std::string_view value) noexcept;
u64 hash_combine(u64 lhs, u64 rhs) noexcept;

}  // namespace render::core
