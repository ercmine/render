#include "engine/serialization/binary_writer.hpp"

#include <stdexcept>

namespace render::serialization {

void BinaryWriter::write_u16(const core::u16 value) {
  bytes_.push_back(static_cast<core::u8>(value & 0xFFU));
  bytes_.push_back(static_cast<core::u8>((value >> 8U) & 0xFFU));
}

void BinaryWriter::write_u32(const core::u32 value) {
  bytes_.push_back(static_cast<core::u8>(value & 0xFFU));
  bytes_.push_back(static_cast<core::u8>((value >> 8U) & 0xFFU));
  bytes_.push_back(static_cast<core::u8>((value >> 16U) & 0xFFU));
  bytes_.push_back(static_cast<core::u8>((value >> 24U) & 0xFFU));
}

void BinaryWriter::write_u64(const core::u64 value) {
  for (core::u32 shift = 0; shift < 64U; shift += 8U) {
    bytes_.push_back(static_cast<core::u8>((value >> shift) & 0xFFULL));
  }
}

void BinaryWriter::write_raw_bytes(const std::span<const core::u8> bytes) {
  bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
}

void BinaryWriter::write_bytes(const std::span<const core::u8> bytes) {
  if (bytes.size() > kMaxContainerLength) {
    throw std::runtime_error("byte blob exceeds canonical maximum length");
  }
  write_u32(static_cast<core::u32>(bytes.size()));
  write_raw_bytes(bytes);
}

void BinaryWriter::write_string(const std::string_view text) {
  const auto* begin = reinterpret_cast<const core::u8*>(text.data());
  write_bytes(std::span<const core::u8>(begin, text.size()));
}

}  // namespace render::serialization
