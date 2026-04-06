#pragma once

#include "engine/serialization/bytes.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace render::serialization {

class BinaryWriter {
public:
  void write_u8(core::u8 value) { bytes_.push_back(value); }
  void write_bool(bool value) { write_u8(value ? 1U : 0U); }
  void write_u16(core::u16 value);
  void write_u32(core::u32 value);
  void write_u64(core::u64 value);
  void write_i32(core::i32 value) { write_u32(std::bit_cast<core::u32>(value)); }
  void write_i64(core::i64 value) { write_u64(std::bit_cast<core::u64>(value)); }
  void write_f32(core::f32 value) { write_u32(std::bit_cast<core::u32>(value)); }
  void write_f64(core::f64 value) { write_u64(std::bit_cast<core::u64>(value)); }

  void write_raw_bytes(std::span<const core::u8> bytes);
  void write_bytes(std::span<const core::u8> bytes);
  void write_string(std::string_view text);

  template <typename Enum>
  void write_enum_u32(Enum value) {
    static_assert(std::is_enum_v<Enum>);
    write_u32(static_cast<core::u32>(value));
  }

  template <typename T, typename Fn>
  void write_vector(const std::vector<T>& values, Fn&& write_value) {
    write_u32(static_cast<core::u32>(values.size()));
    for (const T& value : values) {
      write_value(*this, value);
    }
  }

  template <typename T, std::size_t N, typename Fn>
  void write_array(const std::array<T, N>& values, Fn&& write_value) {
    for (const T& value : values) {
      write_value(*this, value);
    }
  }

  template <typename T, typename Fn>
  void write_optional(const std::optional<T>& value, Fn&& write_value) {
    write_bool(value.has_value());
    if (value.has_value()) {
      write_value(*this, *value);
    }
  }

  template <typename K, typename V, typename Fn>
  void write_map_sorted(const std::unordered_map<K, V>& values, Fn&& write_pair) {
    std::vector<std::pair<K, V>> ordered(values.begin(), values.end());
    std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
    write_u32(static_cast<core::u32>(ordered.size()));
    for (const auto& pair : ordered) {
      write_pair(*this, pair.first, pair.second);
    }
  }

  template <typename T, typename Fn>
  void write_set_sorted(const std::unordered_set<T>& values, Fn&& write_value) {
    std::vector<T> ordered(values.begin(), values.end());
    std::sort(ordered.begin(), ordered.end());
    write_u32(static_cast<core::u32>(ordered.size()));
    for (const T& value : ordered) {
      write_value(*this, value);
    }
  }

  [[nodiscard]] const ByteBuffer& bytes() const noexcept { return bytes_; }

private:
  ByteBuffer bytes_{};
};

}  // namespace render::serialization
