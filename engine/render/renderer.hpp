#pragma once

#include "engine/render/buffer_types.hpp"
#include "engine/render/debug_renderer.hpp"
#include "engine/render/draw_submission.hpp"
#include "engine/render/renderer_types.hpp"
#include "engine/render/shader_types.hpp"
#include "engine/render/texture_types.hpp"

#include <span>

namespace render::platform {
class PlatformRuntime;
}

namespace render::rendering {

class Renderer {
public:
  Renderer();
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  [[nodiscard]] bool initialize(const RendererConfig& config, const platform::PlatformRuntime& runtime);
  void shutdown();

  [[nodiscard]] bool is_initialized() const noexcept;
  [[nodiscard]] bool is_ready() const noexcept;
  [[nodiscard]] bool can_render() const noexcept;
  [[nodiscard]] RendererLifecycleState state() const noexcept;
  [[nodiscard]] RendererStatus status() const noexcept;
  [[nodiscard]] RendererCaps capabilities() const;
  [[nodiscard]] RendererBackend backend() const noexcept;

  [[nodiscard]] bool begin_frame();
  [[nodiscard]] bool end_frame();
  void request_resize(std::uint32_t width, std::uint32_t height);
  [[nodiscard]] bool try_recover();
  void resize(std::uint32_t width, std::uint32_t height);

  void set_debug_enabled(bool enabled);
  [[nodiscard]] bool set_debug_mode(RendererDebugMode mode);
  [[nodiscard]] RendererDebugMode debug_mode() const noexcept;
  [[nodiscard]] bool set_debug_mode_from_index(std::uint8_t index);
  [[nodiscard]] bool cycle_debug_mode();
  void set_debug_program_overrides(const RendererDebugProgramOverrides& overrides);
  void set_debug_counters(const RendererDebugCounters& counters);
  void add_debug_pass_timing(const RendererPassTiming& timing);
  [[nodiscard]] RendererDebugSnapshot debug_snapshot() const;
  void set_view(ViewId view, const ViewDescription& desc);
  void set_view_transform(ViewId view, std::span<const float, 16> view_transform, std::span<const float, 16> projection);

  [[nodiscard]] MeshBufferHandle create_mesh_buffer(const MeshBufferDescription& desc);
  [[nodiscard]] bool update_mesh_buffer(MeshBufferHandle handle, const MeshBufferUpdate& update);

  [[nodiscard]] IndexBufferHandle create_index_buffer(const IndexBufferDescription& desc);
  [[nodiscard]] bool update_index_buffer(IndexBufferHandle handle, const IndexBufferUpdate& update);

  [[nodiscard]] InstanceBufferHandle create_instance_buffer(const InstanceBufferDescription& desc);
  [[nodiscard]] bool update_instance_buffer(InstanceBufferHandle handle, const InstanceBufferUpdate& update);

  [[nodiscard]] ProgramHandle create_program(const ShaderProgramDescription& desc);
  [[nodiscard]] TextureHandle create_solid_color_texture(const SolidColorTextureDescription& desc);

  void destroy_buffer(MeshBufferHandle handle);
  void destroy_buffer(IndexBufferHandle handle);
  void destroy_buffer(InstanceBufferHandle handle);
  void destroy_program(ProgramHandle handle);
  void destroy_texture(TextureHandle handle);

  void submit(ViewId view, const DrawSubmission& submission);
  void submit_instanced(ViewId view, const DrawSubmission& submission, std::span<const float> transforms);

private:
  struct Impl;
  Impl* impl_;
};

}  // namespace render::rendering
