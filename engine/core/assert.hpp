#pragma once

#include "engine/core/log.hpp"

#include <source_location>

namespace render::core {

[[noreturn]] void report_assert_failure(const char* expression, const char* message, std::source_location source);

void verify_or_log(bool condition, const char* expression, const char* message, std::source_location source);

}  // namespace render::core

#ifndef NDEBUG
#define RENDER_ASSERT(expr, msg)                                                                            \
  do {                                                                                                      \
    if (!(expr)) {                                                                                          \
      ::render::core::report_assert_failure(#expr, msg, std::source_location::current());                 \
    }                                                                                                       \
  } while (false)
#else
#define RENDER_ASSERT(expr, msg) ((void)0)
#endif

#define RENDER_VERIFY(expr, msg)                                                                            \
  do {                                                                                                      \
    ::render::core::verify_or_log(static_cast<bool>(expr), #expr, msg, std::source_location::current());  \
  } while (false)
