#pragma once

#include <string>

namespace render::serialization {

enum class ErrorCode {
  None = 0,
  UnexpectedEof,
  InvalidData,
  InvalidLength,
  InvalidUtf8,
  UnknownSchema,
  UnsupportedVersion,
  IncompatibleEnvelope,
  MigrationUnavailable,
};

struct Error {
  ErrorCode code{ErrorCode::None};
  std::string message;
  std::size_t offset{0};

  [[nodiscard]] explicit operator bool() const noexcept { return code != ErrorCode::None; }
};

}  // namespace render::serialization
