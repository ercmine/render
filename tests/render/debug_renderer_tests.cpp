#include "engine/render/debug_renderer.hpp"

#include <cassert>

int main() {
  using namespace render::rendering;

  RendererDebugState state;
  assert(state.mode() == RendererDebugMode::Disabled);
  assert(state.set_mode(RendererDebugMode::Normals));
  assert(state.mode() == RendererDebugMode::Normals);
  assert(!state.set_mode_from_index(44));
  assert(state.set_mode_from_index(0));
  assert(state.mode() == RendererDebugMode::Disabled);
  assert(state.cycle_next_mode());
  assert(state.mode() == RendererDebugMode::Wireframe);

  RendererDebugSnapshot baseline{};
  baseline.backend = RendererBackend::Vulkan;
  baseline.backbuffer_width = 1920;
  baseline.backbuffer_height = 1080;
  baseline.frame_time_ms = 16;
  state.set_gpu_timing_supported(false);
  state.begin_frame(baseline);
  state.set_counters(RendererDebugCounters{
    .visible_directional_lights = 1,
    .visible_point_lights = 8,
    .submitted_draws = 120,
    .instanced_draws = 6,
    .submitted_instances = 300,
  });
  state.add_pass_timing(RendererPassTiming{.pass_name = "main-lit-pass", .cpu_ms = 1.2F, .gpu_ms = std::nullopt});
  const RendererDebugSnapshot snapshot = state.snapshot();
  assert(snapshot.backbuffer_width == 1920);
  assert(snapshot.counters.submitted_draws == 120);
  assert(snapshot.timings.size() == 1);
  assert(snapshot.timings[0].pass_name == "main-lit-pass");
  assert(!snapshot.timings[0].gpu_ms.has_value());

  const auto lines = state.build_overlay_lines();
  assert(lines.size() >= 3);

  return 0;
}
