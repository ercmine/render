#include "engine/core/log.hpp"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace render::core {

namespace {
class ConsoleSink final : public ILogSink {
public:
  void write(const LogMessage& message) override {
    std::ostream& out = (message.level == LogLevel::Error || message.level == LogLevel::Fatal) ? std::cerr : std::cout;
    const auto tt = std::chrono::system_clock::to_time_t(message.timestamp);
    out << std::put_time(std::localtime(&tt), "%H:%M:%S") << " [" << to_string(message.level) << "] [" << message.category
        << "] " << message.text << " (" << message.source.file_name() << ':' << message.source.line() << ")\n";
  }
};

std::mutex g_sink_mutex;
std::vector<std::shared_ptr<ILogSink>> g_sinks;

void ensure_default_sink() {
  if (g_sinks.empty()) {
    g_sinks.push_back(std::make_shared<ConsoleSink>());
  }
}
}  // namespace

void add_log_sink(std::shared_ptr<ILogSink> sink) {
  std::lock_guard lock(g_sink_mutex);
  ensure_default_sink();
  g_sinks.push_back(std::move(sink));
}

void clear_log_sinks() {
  std::lock_guard lock(g_sink_mutex);
  g_sinks.clear();
}

void write_log(const LogMessage& message) {
  std::lock_guard lock(g_sink_mutex);
  ensure_default_sink();
  for (const auto& sink : g_sinks) {
    sink->write(message);
  }
}

std::string to_string(const LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return "trace";
    case LogLevel::Debug:
      return "debug";
    case LogLevel::Info:
      return "info";
    case LogLevel::Warn:
      return "warn";
    case LogLevel::Error:
      return "error";
    case LogLevel::Fatal:
      return "fatal";
  }
  return "unknown";
}

}  // namespace render::core
