#include "engine/render/renderer.hpp"

#include <cassert>

int main() {
  using namespace render::rendering;

  {
    RendererConfig config{};
    config.width = 0;
    config.height = 720;
    const RendererConfigValidation validation = validate_renderer_config(config);
    assert(!validation.valid);
  }

  {
    RendererConfig config{};
    config.width = 1280;
    config.height = 720;
    config.min_frame_time_ms = 16;
    const RendererConfigValidation validation = validate_renderer_config(config);
    assert(validation.valid);
  }

  assert(is_explicit_backend_request(RendererBackend::Vulkan));
  assert(!is_explicit_backend_request(RendererBackend::Auto));
  assert(to_string(RendererBackend::OpenGL) != nullptr);
  assert(to_string(RendererLifecycleState::Ready) != nullptr);

  Renderer renderer;
  assert(renderer.state() == RendererLifecycleState::Uninitialized);
  assert(!renderer.is_ready());
  assert(!renderer.can_render());
  assert(renderer.debug_mode() == RendererDebugMode::Disabled);
  assert(renderer.set_debug_mode(RendererDebugMode::Depth));
  assert(renderer.debug_mode() == RendererDebugMode::Depth);
  assert(!renderer.set_debug_mode_from_index(255));
  assert(!renderer.begin_frame());
  assert(!renderer.end_frame());

  return 0;
}
