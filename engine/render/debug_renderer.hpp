#pragma once

#include "engine/render/renderer_types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace render::rendering {

enum class RendererDebugMode : std::uint8_t {
  Disabled = 0,
  Wireframe,
  Normals,
  Albedo,
  Depth,
  LightVolumes,
  Overdraw,
  GpuTiming,
};

constexpr std::size_t kRendererDebugModeCount = 8;

struct RendererDebugCounters {
  std::uint32_t visible_directional_lights{0};
  std::uint32_t visible_point_lights{0};
  std::uint32_t submitted_draws{0};
  std::uint32_t instanced_draws{0};
  std::uint32_t submitted_instances{0};
  std::uint32_t vfx_active_effects{0};
  std::uint32_t vfx_active_particles{0};
  std::uint32_t vfx_draw_calls{0};
  std::uint32_t vfx_instance_uploads{0};
};

struct RendererPassTiming {
  std::string pass_name{};
  float cpu_ms{0.0F};
  std::optional<float> gpu_ms{};
};

struct RendererDebugSnapshot {
  RendererDebugMode mode{RendererDebugMode::Disabled};
  RendererBackend backend{RendererBackend::Auto};
  std::uint32_t backbuffer_width{0};
  std::uint32_t backbuffer_height{0};
  std::uint32_t frame_time_ms{0};
  RendererDebugCounters counters{};
  bool gpu_timing_supported{false};
  std::vector<RendererPassTiming> timings{};
};

struct RendererDebugProgramOverrides {
  ProgramHandle normals{};
  ProgramHandle albedo{};
  ProgramHandle depth{};
  ProgramHandle overdraw{};
};

[[nodiscard]] const char* to_string(RendererDebugMode mode) noexcept;
[[nodiscard]] std::optional<RendererDebugMode> renderer_debug_mode_from_index(std::uint8_t index) noexcept;

class RendererDebugState {
public:
  [[nodiscard]] RendererDebugMode mode() const noexcept;
  [[nodiscard]] bool set_mode(RendererDebugMode mode);
  [[nodiscard]] bool set_mode_from_index(std::uint8_t index);
  [[nodiscard]] bool cycle_next_mode();

  void set_gpu_timing_supported(bool supported) noexcept;
  [[nodiscard]] bool gpu_timing_supported() const noexcept;
  void set_program_overrides(const RendererDebugProgramOverrides& overrides) noexcept;
  [[nodiscard]] const RendererDebugProgramOverrides& program_overrides() const noexcept;

  void begin_frame(const RendererDebugSnapshot& baseline);
  void set_counters(const RendererDebugCounters& counters);
  void add_pass_timing(const RendererPassTiming& timing);
  [[nodiscard]] RendererDebugSnapshot snapshot() const;
  [[nodiscard]] std::vector<std::string> build_overlay_lines() const;

private:
  RendererDebugMode mode_{RendererDebugMode::Disabled};
  bool gpu_timing_supported_{false};
  RendererDebugProgramOverrides program_overrides_{};
  RendererDebugSnapshot snapshot_{};
};

}  // namespace render::rendering
