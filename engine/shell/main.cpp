#include "engine/platform/platform_log.hpp"
#include "engine/platform/platform_runtime.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

int main() {
  render::platform::PlatformRuntime runtime;

  render::platform::RuntimeConfig config{};
  config.app_name = "render-shell";
  config.org_name = "render";
  config.window.title = "render :: Statement 3 shell";
  config.window.width = 1280;
  config.window.height = 720;
  config.window.resizable = true;

  if (!runtime.initialize(config)) {
    render::platform::log::error("Platform runtime failed to initialize");
    return 1;
  }

  constexpr int kSampleRate = 48000;
  constexpr float kToneFrequency = 220.0F;
  constexpr std::size_t kFramesPerChunk = 512;
  std::vector<float> tone;
  tone.resize(kFramesPerChunk * 2U);
  float phase = 0.0F;

  while (!runtime.should_quit()) {
    runtime.begin_frame();
    runtime.pump_events();
    runtime.end_frame();

    if (runtime.audio().is_initialized()) {
      const float phase_step = (2.0F * 3.1415926535F * kToneFrequency) / static_cast<float>(kSampleRate);
      for (std::size_t i = 0; i < kFramesPerChunk; ++i) {
        const float sample = std::sin(phase) * 0.05F;
        tone[i * 2U] = sample;
        tone[i * 2U + 1U] = sample;
        phase += phase_step;
      }
      runtime.audio().queue_interleaved_float_samples(tone.data(), tone.size());
    }

    render::platform::PlatformRuntime::sleep_for_milliseconds(1);
  }

  runtime.shutdown();
  return 0;
}
