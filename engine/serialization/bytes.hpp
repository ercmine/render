#pragma once

#include "engine/core/types.hpp"

#include <vector>

namespace render::serialization {

using ByteBuffer = std::vector<core::u8>;

inline constexpr core::u32 kMaxContainerLength = 1U << 24;

}  // namespace render::serialization
