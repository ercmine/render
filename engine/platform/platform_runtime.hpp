#pragma once

#include "engine/platform/platform_types.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace render::platform {

class AudioService {
public:
  bool is_initialized() const noexcept;
  bool queue_interleaved_float_samples(const float* samples, std::size_t sample_count) noexcept;

private:
  friend class PlatformRuntime;
  void* stream_{nullptr};
  bool initialized_{false};
};

class PlatformRuntime {
public:
  PlatformRuntime();
  ~PlatformRuntime();

  PlatformRuntime(const PlatformRuntime&) = delete;
  PlatformRuntime& operator=(const PlatformRuntime&) = delete;

  [[nodiscard]] bool initialize(const RuntimeConfig& config);
  void shutdown();

  void begin_frame();
  void pump_events();
  void end_frame();

  [[nodiscard]] bool should_quit() const noexcept;
  [[nodiscard]] const InputFrame& input() const noexcept;
  [[nodiscard]] const std::vector<GamepadState>& gamepads() const noexcept;
  [[nodiscard]] const TimingState& timing() const noexcept;
  [[nodiscard]] const Paths& paths() const noexcept;
  [[nodiscard]] const WindowState& window_state() const noexcept;
  [[nodiscard]] AudioService& audio() noexcept;

  [[nodiscard]] void* native_window_handle() const noexcept;

  static void sleep_for_milliseconds(std::uint32_t milliseconds) noexcept;

private:
  struct Impl;
  Impl* impl_;
};

}  // namespace render::platform
