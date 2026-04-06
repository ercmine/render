#pragma once

#include <chrono>
#include <memory>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace render::core {

enum class LogLevel { Trace, Debug, Info, Warn, Error, Fatal };

struct LogMessage {
  std::chrono::system_clock::time_point timestamp{};
  LogLevel level{LogLevel::Info};
  std::string category{"core"};
  std::string text{};
  std::source_location source = std::source_location::current();
};

class ILogSink {
public:
  virtual ~ILogSink() = default;
  virtual void write(const LogMessage& message) = 0;
};

void add_log_sink(std::shared_ptr<ILogSink> sink);
void clear_log_sinks();
void write_log(const LogMessage& message);

std::string to_string(LogLevel level);

namespace detail {
template <typename... Args>
std::string concat_message(Args&&... args) {
  std::ostringstream oss;
  (oss << ... << std::forward<Args>(args));
  return oss.str();
}
}

template <typename... Args>
void log(LogLevel level, std::string_view category, Args&&... args) {
  write_log({
      .timestamp = std::chrono::system_clock::now(),
      .level = level,
      .category = std::string(category),
      .text = detail::concat_message(std::forward<Args>(args)...),
      .source = std::source_location::current(),
  });
}

#define RENDER_LOG_TRACE(category, ...) ::render::core::log(::render::core::LogLevel::Trace, category, __VA_ARGS__)
#define RENDER_LOG_DEBUG(category, ...) ::render::core::log(::render::core::LogLevel::Debug, category, __VA_ARGS__)
#define RENDER_LOG_INFO(category, ...) ::render::core::log(::render::core::LogLevel::Info, category, __VA_ARGS__)
#define RENDER_LOG_WARN(category, ...) ::render::core::log(::render::core::LogLevel::Warn, category, __VA_ARGS__)
#define RENDER_LOG_ERROR(category, ...) ::render::core::log(::render::core::LogLevel::Error, category, __VA_ARGS__)
#define RENDER_LOG_FATAL(category, ...) ::render::core::log(::render::core::LogLevel::Fatal, category, __VA_ARGS__)

}  // namespace render::core
