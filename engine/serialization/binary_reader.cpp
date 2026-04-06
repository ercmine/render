#include "engine/serialization/binary_reader.hpp"

namespace render::serialization {

bool BinaryReader::read_u8(core::u8& out) {
  if (!require(1)) {
    return false;
  }
  out = bytes_[cursor_++];
  return true;
}

bool BinaryReader::read_bool(bool& out) {
  core::u8 value = 0;
  if (!read_u8(value)) {
    return false;
  }
  if (value > 1U) {
    fail(ErrorCode::InvalidData, "invalid bool encoding");
    return false;
  }
  out = (value == 1U);
  return true;
}

bool BinaryReader::read_u16(core::u16& out) {
  if (!require(2)) {
    return false;
  }
  out = static_cast<core::u16>(bytes_[cursor_]) | (static_cast<core::u16>(bytes_[cursor_ + 1]) << 8U);
  cursor_ += 2;
  return true;
}

bool BinaryReader::read_u32(core::u32& out) {
  if (!require(4)) {
    return false;
  }
  out = static_cast<core::u32>(bytes_[cursor_]) | (static_cast<core::u32>(bytes_[cursor_ + 1]) << 8U) |
        (static_cast<core::u32>(bytes_[cursor_ + 2]) << 16U) | (static_cast<core::u32>(bytes_[cursor_ + 3]) << 24U);
  cursor_ += 4;
  return true;
}

bool BinaryReader::read_u64(core::u64& out) {
  if (!require(8)) {
    return false;
  }
  out = 0ULL;
  for (core::u32 shift = 0; shift < 64U; shift += 8U) {
    out |= static_cast<core::u64>(bytes_[cursor_++]) << shift;
  }
  return true;
}

bool BinaryReader::read_i32(core::i32& out) {
  core::u32 bits = 0;
  if (!read_u32(bits)) {
    return false;
  }
  out = std::bit_cast<core::i32>(bits);
  return true;
}

bool BinaryReader::read_i64(core::i64& out) {
  core::u64 bits = 0;
  if (!read_u64(bits)) {
    return false;
  }
  out = std::bit_cast<core::i64>(bits);
  return true;
}

bool BinaryReader::read_f32(core::f32& out) {
  core::u32 bits = 0;
  if (!read_u32(bits)) {
    return false;
  }
  out = std::bit_cast<core::f32>(bits);
  return true;
}

bool BinaryReader::read_f64(core::f64& out) {
  core::u64 bits = 0;
  if (!read_u64(bits)) {
    return false;
  }
  out = std::bit_cast<core::f64>(bits);
  return true;
}

bool BinaryReader::read_bytes(ByteBuffer& out) {
  core::u32 size = 0;
  if (!read_u32(size)) {
    return false;
  }
  if (size > kMaxContainerLength) {
    fail(ErrorCode::InvalidLength, "byte blob length exceeds canonical maximum");
    return false;
  }
  if (!require(size)) {
    return false;
  }
  out.assign(bytes_.begin() + static_cast<std::ptrdiff_t>(cursor_),
             bytes_.begin() + static_cast<std::ptrdiff_t>(cursor_ + size));
  cursor_ += size;
  return true;
}

bool BinaryReader::read_string(std::string& out) {
  ByteBuffer blob;
  if (!read_bytes(blob)) {
    return false;
  }
  out.assign(reinterpret_cast<const char*>(blob.data()), blob.size());
  return true;
}

bool BinaryReader::require(const std::size_t count) {
  if (cursor_ + count > bytes_.size()) {
    fail(ErrorCode::UnexpectedEof, "truncated payload");
    return false;
  }
  return true;
}

void BinaryReader::fail(const ErrorCode code, std::string message) {
  if (error_.code == ErrorCode::None) {
    error_ = Error{code, std::move(message), cursor_};
  }
}

}  // namespace render::serialization
