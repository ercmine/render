#include "engine/render/debug_renderer.hpp"

#include <sstream>

namespace render::rendering {
namespace {

const char* backend_label(const RendererBackend backend) noexcept {
  switch (backend) {
    case RendererBackend::Auto: return "auto";
    case RendererBackend::Noop: return "noop";
    case RendererBackend::Direct3D11: return "d3d11";
    case RendererBackend::Direct3D12: return "d3d12";
    case RendererBackend::Metal: return "metal";
    case RendererBackend::Vulkan: return "vulkan";
    case RendererBackend::OpenGL: return "opengl";
    default: return "unknown";
  }
}

}  // namespace

const char* to_string(const RendererDebugMode mode) noexcept {
  switch (mode) {
    case RendererDebugMode::Disabled: return "disabled";
    case RendererDebugMode::Wireframe: return "wireframe";
    case RendererDebugMode::Normals: return "normals";
    case RendererDebugMode::Albedo: return "albedo";
    case RendererDebugMode::Depth: return "depth";
    case RendererDebugMode::LightVolumes: return "light-volumes";
    case RendererDebugMode::Overdraw: return "overdraw";
    case RendererDebugMode::GpuTiming: return "gpu-timing";
    default: return "unknown";
  }
}

std::optional<RendererDebugMode> renderer_debug_mode_from_index(const std::uint8_t index) noexcept {
  if (index >= kRendererDebugModeCount) {
    return std::nullopt;
  }
  return static_cast<RendererDebugMode>(index);
}

RendererDebugMode RendererDebugState::mode() const noexcept {
  return mode_;
}

bool RendererDebugState::set_mode(const RendererDebugMode mode) {
  if (mode_ == mode) {
    return true;
  }
  mode_ = mode;
  snapshot_.mode = mode_;
  return true;
}

bool RendererDebugState::set_mode_from_index(const std::uint8_t index) {
  const std::optional<RendererDebugMode> mode = renderer_debug_mode_from_index(index);
  if (!mode.has_value()) {
    return false;
  }
  return set_mode(*mode);
}

bool RendererDebugState::cycle_next_mode() {
  const auto next = static_cast<std::uint8_t>((static_cast<std::uint8_t>(mode_) + 1U) % kRendererDebugModeCount);
  return set_mode(static_cast<RendererDebugMode>(next));
}

void RendererDebugState::set_gpu_timing_supported(const bool supported) noexcept {
  gpu_timing_supported_ = supported;
  snapshot_.gpu_timing_supported = supported;
}

bool RendererDebugState::gpu_timing_supported() const noexcept {
  return gpu_timing_supported_;
}

void RendererDebugState::set_program_overrides(const RendererDebugProgramOverrides& overrides) noexcept {
  program_overrides_ = overrides;
}

const RendererDebugProgramOverrides& RendererDebugState::program_overrides() const noexcept {
  return program_overrides_;
}

void RendererDebugState::begin_frame(const RendererDebugSnapshot& baseline) {
  snapshot_ = baseline;
  snapshot_.mode = mode_;
  snapshot_.gpu_timing_supported = gpu_timing_supported_;
  snapshot_.timings.clear();
}

void RendererDebugState::set_counters(const RendererDebugCounters& counters) {
  snapshot_.counters = counters;
}

void RendererDebugState::add_pass_timing(const RendererPassTiming& timing) {
  snapshot_.timings.push_back(timing);
}

RendererDebugSnapshot RendererDebugState::snapshot() const {
  return snapshot_;
}

std::vector<std::string> RendererDebugState::build_overlay_lines() const {
  std::vector<std::string> lines;
  lines.reserve(10U + snapshot_.timings.size());

  std::ostringstream header;
  header << "render debug mode=" << to_string(snapshot_.mode)
         << " backend=" << backend_label(snapshot_.backend)
         << " backbuffer=" << snapshot_.backbuffer_width << "x" << snapshot_.backbuffer_height;
  lines.push_back(header.str());

  std::ostringstream counts;
  counts << "lights dir=" << snapshot_.counters.visible_directional_lights
         << " point=" << snapshot_.counters.visible_point_lights
         << " draws=" << snapshot_.counters.submitted_draws
         << " instanced=" << snapshot_.counters.instanced_draws
         << " instances=" << snapshot_.counters.submitted_instances
         << " frame_ms=" << snapshot_.frame_time_ms;
  lines.push_back(counts.str());

  std::ostringstream vfx_counts;
  vfx_counts << "vfx effects=" << snapshot_.counters.vfx_active_effects
             << " particles=" << snapshot_.counters.vfx_active_particles
             << " draws=" << snapshot_.counters.vfx_draw_calls
             << " uploads=" << snapshot_.counters.vfx_instance_uploads;
  lines.push_back(vfx_counts.str());

  if (!snapshot_.gpu_timing_supported) {
    lines.push_back("gpu timing unavailable (using cpu pass timings)");
  }

  for (const RendererPassTiming& timing : snapshot_.timings) {
    std::ostringstream pass;
    pass << timing.pass_name << " cpu=" << timing.cpu_ms << "ms";
    if (timing.gpu_ms.has_value()) {
      pass << " gpu=" << *timing.gpu_ms << "ms";
    } else {
      pass << " gpu=n/a";
    }
    lines.push_back(pass.str());
  }

  if (snapshot_.mode == RendererDebugMode::LightVolumes) {
    lines.push_back("light-volumes mode overlays per-light influence diagnostics");
  } else if (snapshot_.mode == RendererDebugMode::Depth) {
    lines.push_back("depth mode uses linearized near/far remap in debug shader");
  } else if (snapshot_.mode == RendererDebugMode::Overdraw) {
    lines.push_back("overdraw mode approximates cost via additive no-depth accumulation");
  }

  return lines;
}

}  // namespace render::rendering
