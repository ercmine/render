#include "engine/core/assert.hpp"

#include <cstdlib>

namespace render::core {

[[noreturn]] void report_assert_failure(const char* expression, const char* message, const std::source_location source) {
  write_log({
      .timestamp = std::chrono::system_clock::now(),
      .level = LogLevel::Fatal,
      .category = "assert",
      .text = detail::concat_message("Assertion failed: ", expression, " | ", message),
      .source = source,
  });
  std::abort();
}

void verify_or_log(const bool condition, const char* expression, const char* message, const std::source_location source) {
  if (!condition) {
    write_log({
        .timestamp = std::chrono::system_clock::now(),
        .level = LogLevel::Error,
        .category = "verify",
        .text = detail::concat_message("Verification failed: ", expression, " | ", message),
        .source = source,
    });
  }
}

}  // namespace render::core
