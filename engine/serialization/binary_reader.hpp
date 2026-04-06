#pragma once

#include "engine/serialization/bytes.hpp"
#include "engine/serialization/errors.hpp"

#include <array>
#include <bit>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace render::serialization {

class BinaryReader {
public:
  explicit BinaryReader(std::span<const core::u8> bytes) : bytes_(bytes) {}

  [[nodiscard]] bool read_u8(core::u8& out);
  [[nodiscard]] bool read_bool(bool& out);
  [[nodiscard]] bool read_u16(core::u16& out);
  [[nodiscard]] bool read_u32(core::u32& out);
  [[nodiscard]] bool read_u64(core::u64& out);
  [[nodiscard]] bool read_i32(core::i32& out);
  [[nodiscard]] bool read_i64(core::i64& out);
  [[nodiscard]] bool read_f32(core::f32& out);
  [[nodiscard]] bool read_f64(core::f64& out);

  [[nodiscard]] bool read_bytes(ByteBuffer& out);
  [[nodiscard]] bool read_string(std::string& out);

  template <typename Enum>
  [[nodiscard]] bool read_enum_u32(Enum& out) {
    static_assert(std::is_enum_v<Enum>);
    core::u32 raw = 0;
    if (!read_u32(raw)) {
      return false;
    }
    out = static_cast<Enum>(raw);
    return true;
  }

  template <typename T, typename Fn>
  [[nodiscard]] bool read_vector(std::vector<T>& out, Fn&& read_value) {
    core::u32 count = 0;
    if (!read_u32(count)) {
      return false;
    }
    if (count > kMaxContainerLength) {
      fail(ErrorCode::InvalidLength, "vector length exceeds canonical maximum");
      return false;
    }
    out.clear();
    out.reserve(count);
    for (core::u32 i = 0; i < count; ++i) {
      T value{};
      if (!read_value(*this, value)) {
        return false;
      }
      out.push_back(std::move(value));
    }
    return true;
  }

  template <typename T, std::size_t N, typename Fn>
  [[nodiscard]] bool read_array(std::array<T, N>& out, Fn&& read_value) {
    for (T& value : out) {
      if (!read_value(*this, value)) {
        return false;
      }
    }
    return true;
  }

  template <typename T, typename Fn>
  [[nodiscard]] bool read_optional(std::optional<T>& out, Fn&& read_value) {
    bool present = false;
    if (!read_bool(present)) {
      return false;
    }
    if (!present) {
      out.reset();
      return true;
    }
    T value{};
    if (!read_value(*this, value)) {
      return false;
    }
    out = std::move(value);
    return true;
  }

  [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - cursor_; }
  [[nodiscard]] std::size_t offset() const noexcept { return cursor_; }
  [[nodiscard]] const Error& error() const noexcept { return error_; }

private:
  [[nodiscard]] bool require(std::size_t count);
  void fail(ErrorCode code, std::string message);

  std::span<const core::u8> bytes_;
  std::size_t cursor_{0};
  Error error_{};
};

}  // namespace render::serialization
