#pragma once

#include <cstdint>
#include <limits>
#include <optional>
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

enum class RendererLifecycleState : std::uint8_t {
  Uninitialized = 0,
  Initializing,
  Ready,
  Resizing,
  Recovering,
  ShuttingDown,
  Failed,
};

enum class RendererResetFlag : std::uint32_t {
  None = 0,
  VSync = 1U << 0U,
};

struct RendererConfig {
  RendererBackend backend{RendererBackend::Auto};
  std::uint32_t width{1280};
  std::uint32_t height{720};
  bool debug{false};
  bool vsync{true};
  std::uint32_t reset_flags{0};
  std::uint32_t min_frame_time_ms{0};
  bool allow_automatic_recovery{true};
};

struct RendererCaps {
  std::string renderer_name{};
  std::uint64_t supported_features{0};
  std::uint16_t max_views{0};
  std::uint32_t max_texture_size{0};
  bool homogeneous_depth{false};
  bool origin_bottom_left{false};
};

struct RendererFrameStats {
  std::uint64_t frame_count{0};
  std::uint32_t backbuffer_width{0};
  std::uint32_t backbuffer_height{0};
  std::uint32_t last_frame_time_ms{0};
  bool vsync_enabled{true};
  bool frame_active{false};
};

struct RendererStatus {
  RendererLifecycleState state{RendererLifecycleState::Uninitialized};
  RendererBackend selected_backend{RendererBackend::Auto};
  RendererFrameStats frame{};
  bool device_lost{false};
  bool can_render{false};
};

struct RendererConfigValidation {
  bool valid{false};
  std::string reason{};
};

[[nodiscard]] constexpr std::uint32_t to_mask(const RendererResetFlag flag) noexcept {
  return static_cast<std::uint32_t>(flag);
}

[[nodiscard]] constexpr bool is_explicit_backend_request(const RendererBackend backend) noexcept {
  return backend != RendererBackend::Auto;
}

[[nodiscard]] const char* to_string(RendererBackend backend) noexcept;
[[nodiscard]] const char* to_string(RendererLifecycleState state) noexcept;
[[nodiscard]] RendererConfigValidation validate_renderer_config(const RendererConfig& config);

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
