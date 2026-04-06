#pragma once

#include "engine/core/hash.hpp"

#include <array>
#include <optional>
#include <string>

namespace render::core {

class Uuid {
public:
  Uuid() = default;
  explicit Uuid(std::array<u8, 16> bytes) : bytes_(bytes) {}

  static Uuid generate_v4();
  static std::optional<Uuid> parse(const std::string& text);

  [[nodiscard]] std::string to_string() const;
  [[nodiscard]] const std::array<u8, 16>& bytes() const noexcept { return bytes_; }
  [[nodiscard]] u64 hash() const noexcept;

  friend bool operator==(const Uuid&, const Uuid&) noexcept = default;

private:
  std::array<u8, 16> bytes_{};
};

}  // namespace render::core

namespace std {

template <>
struct hash<render::core::Uuid> {
  std::size_t operator()(const render::core::Uuid& value) const noexcept { return static_cast<std::size_t>(value.hash()); }
};

}  // namespace std
