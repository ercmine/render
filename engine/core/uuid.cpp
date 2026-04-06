#include "engine/core/uuid.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace render::core {

Uuid Uuid::generate_v4() {
  thread_local std::mt19937_64 rng(
      static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) ^ std::random_device{}());
  std::uniform_int_distribution<u32> dist(0U, 255U);

  std::array<u8, 16> out{};
  for (auto& b : out) {
    b = static_cast<u8>(dist(rng));
  }
  out[6] = static_cast<u8>((out[6] & 0x0FU) | 0x40U);
  out[8] = static_cast<u8>((out[8] & 0x3FU) | 0x80U);
  return Uuid(out);
}

std::optional<Uuid> Uuid::parse(const std::string& text) {
  if (text.size() != 36U) {
    return std::nullopt;
  }

  std::array<u8, 16> out{};
  int byte_index = 0;
  for (std::size_t i = 0; i < text.size();) {
    if (text[i] == '-') {
      ++i;
      continue;
    }
    if (i + 1 >= text.size() || byte_index >= static_cast<int>(out.size())) {
      return std::nullopt;
    }

    const char hex[3] = {text[i], text[i + 1], '\0'};
    unsigned value = 0;
    const auto parse_result = std::from_chars(hex, hex + 2, value, 16);
    if (parse_result.ec != std::errc{}) {
      return std::nullopt;
    }
    out[byte_index++] = static_cast<u8>(value);
    i += 2;
  }

  if (byte_index != 16) {
    return std::nullopt;
  }
  return Uuid(out);
}

std::string Uuid::to_string() const {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (std::size_t i = 0; i < bytes_.size(); ++i) {
    oss << std::setw(2) << static_cast<unsigned>(bytes_[i]);
    if (i == 3 || i == 5 || i == 7 || i == 9) {
      oss << '-';
    }
  }
  return oss.str();
}

u64 Uuid::hash() const noexcept {
  const auto* data = reinterpret_cast<const std::byte*>(bytes_.data());
  return hash_bytes(std::span<const std::byte>(data, bytes_.size()));
}

}  // namespace render::core
