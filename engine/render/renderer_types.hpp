#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace render::platform {
class PlatformRuntime;
}

namespace render::rendering {

constexpr std::uint16_t kInvalidHandle = std::numeric_limits<std::uint16_t>::max();

enum class RendererBackend : std::uint8_t {
  Auto = 0,
  Noop,
  Direct3D11,
  Direct3D12,
  Metal,
  Vulkan,
  OpenGL,
};

struct RendererConfig {
  RendererBackend backend{RendererBackend::Auto};
  std::uint32_t width{1280};
  std::uint32_t height{720};
  bool debug{false};
  bool vsync{true};
};

struct RendererCaps {
  std::string renderer_name{};
  std::uint64_t supported_features{0};
  std::uint16_t max_views{0};
  std::uint32_t max_texture_size{0};
  bool homogeneous_depth{false};
  bool origin_bottom_left{false};
};

struct ViewId {
  std::uint16_t value{0};
};

struct ProgramHandle {
  std::uint16_t idx{kInvalidHandle};
};

struct VertexBufferHandle {
  std::uint16_t idx{kInvalidHandle};
};

struct IndexBufferHandle {
  std::uint16_t idx{kInvalidHandle};
};

struct TextureHandle {
  std::uint16_t idx{kInvalidHandle};
};

struct ClearState {
  std::uint32_t rgba{0x303030ff};
  float depth{1.0F};
  std::uint8_t stencil{0};
  bool clear_color{true};
  bool clear_depth{true};
  bool clear_stencil{false};
};

struct ViewRect {
  std::uint16_t x{0};
  std::uint16_t y{0};
  std::uint16_t width{0};
  std::uint16_t height{0};
};

struct ViewDescription {
  ViewRect rect{};
  ClearState clear{};
};

struct DrawState {
  std::uint64_t state_flags{0};
};

}  // namespace render::rendering
