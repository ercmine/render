#pragma once

#include "engine/render/buffer_types.hpp"
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
  [[nodiscard]] RendererCaps capabilities() const;
  [[nodiscard]] RendererBackend backend() const noexcept;

  void begin_frame();
  void end_frame();
  void resize(std::uint32_t width, std::uint32_t height);

  void set_debug_enabled(bool enabled);
  void set_view(ViewId view, const ViewDescription& desc);
  void set_view_transform(ViewId view, std::span<const float, 16> view_transform, std::span<const float, 16> projection);

  [[nodiscard]] VertexBufferHandle create_vertex_buffer(const VertexBufferDescription& desc);
  [[nodiscard]] IndexBufferHandle create_index_buffer(const IndexBufferDescription& desc);
  [[nodiscard]] ProgramHandle create_program(const ShaderProgramDescription& desc);
  [[nodiscard]] TextureHandle create_solid_color_texture(const SolidColorTextureDescription& desc);

  void destroy_buffer(VertexBufferHandle handle);
  void destroy_buffer(IndexBufferHandle handle);
  void destroy_program(ProgramHandle handle);
  void destroy_texture(TextureHandle handle);

  void submit(ViewId view, const MeshSubmission& mesh_submission);

private:
  struct Impl;
  Impl* impl_;
};

}  // namespace render::rendering
