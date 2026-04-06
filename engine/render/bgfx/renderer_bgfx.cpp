#include "engine/render/renderer.hpp"

#include "engine/platform/platform_log.hpp"
#include "engine/platform/platform_runtime.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bimg/decode.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace render::rendering {
namespace {

constexpr std::uint16_t kMainView = 0;

bgfx::RendererType::Enum to_bgfx_renderer_type(const RendererBackend backend) {
  switch (backend) {
    case RendererBackend::Noop: return bgfx::RendererType::Noop;
    case RendererBackend::Direct3D11: return bgfx::RendererType::Direct3D11;
    case RendererBackend::Direct3D12: return bgfx::RendererType::Direct3D12;
    case RendererBackend::Metal: return bgfx::RendererType::Metal;
    case RendererBackend::Vulkan: return bgfx::RendererType::Vulkan;
    case RendererBackend::OpenGL: return bgfx::RendererType::OpenGL;
    case RendererBackend::Auto:
    default: return bgfx::RendererType::Count;
  }
}

const char* renderer_name(const bgfx::RendererType::Enum type) {
  return bgfx::getRendererName(type);
}

std::string read_file_to_string(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return {};
  }
  std::ostringstream out;
  out << stream.rdbuf();
  return out.str();
}

bgfx::ShaderHandle load_shader(const std::filesystem::path& path) {
  const std::string bytes = read_file_to_string(path);
  if (bytes.empty()) {
    platform::log::error(std::string{"Shader file was empty or missing: "} + path.string());
    return BGFX_INVALID_HANDLE;
  }

  const bgfx::Memory* memory = bgfx::copy(bytes.data(), static_cast<uint32_t>(bytes.size()));
  if (memory == nullptr) {
    platform::log::error(std::string{"Failed to allocate bgfx memory for shader: "} + path.string());
    return BGFX_INVALID_HANDLE;
  }

  return bgfx::createShader(memory);
}

bgfx::Attrib::Enum to_bgfx_attrib(const VertexAttribute attribute) {
  switch (attribute) {
    case VertexAttribute::Position: return bgfx::Attrib::Position;
    case VertexAttribute::Normal: return bgfx::Attrib::Normal;
    case VertexAttribute::Tangent: return bgfx::Attrib::Tangent;
    case VertexAttribute::Color0: return bgfx::Attrib::Color0;
    case VertexAttribute::TexCoord0: return bgfx::Attrib::TexCoord0;
    default: return bgfx::Attrib::Position;
  }
}

bgfx::AttribType::Enum to_bgfx_attrib_type(const AttributeType type) {
  switch (type) {
    case AttributeType::Uint8: return bgfx::AttribType::Uint8;
    case AttributeType::Int16: return bgfx::AttribType::Int16;
    case AttributeType::Half: return bgfx::AttribType::Half;
    case AttributeType::Float:
    default: return bgfx::AttribType::Float;
  }
}

void set_platform_data_from_sdl(bgfx::Init& init, SDL_Window* sdl_window) {
  SDL_PropertiesID props = SDL_GetWindowProperties(sdl_window);
  if (props == 0) {
    return;
  }

#if defined(_WIN32)
  init.platformData.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__APPLE__)
  init.platformData.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#elif defined(__linux__)
  if (void* wl_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr)) {
    init.platformData.ndt = wl_display;
    init.platformData.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
  } else {
    init.platformData.ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    const auto x11_window = static_cast<uint32_t>(
      SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
    init.platformData.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(x11_window));
  }
#endif
}

RendererBackend to_backend(bgfx::RendererType::Enum type) {
  switch (type) {
    case bgfx::RendererType::Noop: return RendererBackend::Noop;
    case bgfx::RendererType::Direct3D11: return RendererBackend::Direct3D11;
    case bgfx::RendererType::Direct3D12: return RendererBackend::Direct3D12;
    case bgfx::RendererType::Metal: return RendererBackend::Metal;
    case bgfx::RendererType::Vulkan: return RendererBackend::Vulkan;
    case bgfx::RendererType::OpenGL: return RendererBackend::OpenGL;
    default: return RendererBackend::Auto;
  }
}

constexpr std::uint32_t minimum_dimension(std::uint32_t value) {
  return std::max<std::uint32_t>(value, 1U);
}

}  // namespace

const char* to_string(const RendererBackend backend) noexcept {
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

const char* to_string(const RendererLifecycleState state) noexcept {
  switch (state) {
    case RendererLifecycleState::Uninitialized: return "uninitialized";
    case RendererLifecycleState::Initializing: return "initializing";
    case RendererLifecycleState::Ready: return "ready";
    case RendererLifecycleState::Resizing: return "resizing";
    case RendererLifecycleState::Recovering: return "recovering";
    case RendererLifecycleState::ShuttingDown: return "shutting-down";
    case RendererLifecycleState::Failed: return "failed";
    default: return "unknown";
  }
}

RendererConfigValidation validate_renderer_config(const RendererConfig& config) {
  if (config.width == 0U || config.height == 0U) {
    return {.valid = false, .reason = "initial dimensions must be greater than zero"};
  }
  if (config.min_frame_time_ms > 1000U) {
    return {.valid = false, .reason = "min_frame_time_ms must be <= 1000"};
  }
  return {.valid = true, .reason = {}};
}

struct Renderer::Impl {
  RendererConfig config{};
  RendererLifecycleState lifecycle{RendererLifecycleState::Uninitialized};
  RendererBackend selected_backend{RendererBackend::Auto};
  std::uint32_t reset_flags{BGFX_RESET_NONE};
  std::uint32_t backbuffer_width{0};
  std::uint32_t backbuffer_height{0};
  std::uint32_t pending_width{0};
  std::uint32_t pending_height{0};
  bool has_pending_resize{false};
  bool frame_active{false};
  bool device_lost{false};
  bool bgfx_initialized{false};
  std::uint64_t frame_count{0};
  std::chrono::steady_clock::time_point frame_begin_time{};
  std::uint32_t last_frame_time_ms{0};

  [[nodiscard]] bool is_initialized() const noexcept {
    return bgfx_initialized;
  }

  [[nodiscard]] bool can_render() const noexcept {
    return lifecycle == RendererLifecycleState::Ready && backbuffer_width > 0U && backbuffer_height > 0U;
  }

  [[nodiscard]] std::uint32_t reset_flags_from_config() const {
    std::uint32_t flags = config.reset_flags;
    if (config.vsync) {
      flags |= BGFX_RESET_VSYNC;
    } else {
      flags &= ~BGFX_RESET_VSYNC;
    }
    return flags;
  }

  void set_lifecycle(const RendererLifecycleState state) {
    lifecycle = state;
    platform::log::info(std::string{"Renderer state -> "} + to_string(lifecycle));
  }

  void queue_resize(const std::uint32_t width, const std::uint32_t height) {
    const std::uint32_t clamped_width = width;
    const std::uint32_t clamped_height = height;
    if (has_pending_resize && pending_width == clamped_width && pending_height == clamped_height) {
      return;
    }

    pending_width = clamped_width;
    pending_height = clamped_height;
    has_pending_resize = true;
  }

  void apply_resize_if_needed() {
    if (!has_pending_resize) {
      return;
    }

    has_pending_resize = false;
    if (pending_width == 0U || pending_height == 0U) {
      backbuffer_width = pending_width;
      backbuffer_height = pending_height;
      platform::log::info("Renderer resize deferred while window is minimized (zero dimension)");
      return;
    }

    set_lifecycle(RendererLifecycleState::Resizing);
    backbuffer_width = pending_width;
    backbuffer_height = pending_height;
    bgfx::reset(backbuffer_width, backbuffer_height, reset_flags);

    std::ostringstream resize_log;
    resize_log << "Renderer resize applied: " << backbuffer_width << "x" << backbuffer_height;
    platform::log::info(resize_log.str());
    set_lifecycle(RendererLifecycleState::Ready);
  }

  RendererStatus make_status() const {
    RendererStatus status{};
    status.state = lifecycle;
    status.selected_backend = selected_backend;
    status.frame.frame_count = frame_count;
    status.frame.backbuffer_width = backbuffer_width;
    status.frame.backbuffer_height = backbuffer_height;
    status.frame.last_frame_time_ms = last_frame_time_ms;
    status.frame.vsync_enabled = (reset_flags & BGFX_RESET_VSYNC) != 0U;
    status.frame.frame_active = frame_active;
    status.device_lost = device_lost;
    status.can_render = can_render();
    return status;
  }
};

Renderer::Renderer() : impl_(new Impl{}) {}
Renderer::~Renderer() {
  shutdown();
  delete impl_;
  impl_ = nullptr;
}

bool Renderer::initialize(const RendererConfig& config, const platform::PlatformRuntime& runtime) {
  if (impl_->lifecycle == RendererLifecycleState::Initializing) {
    platform::log::warn("Renderer initialization requested while already initializing");
    return false;
  }

  if (impl_->lifecycle == RendererLifecycleState::Ready) {
    platform::log::warn("Renderer initialization requested while already ready");
    return true;
  }

  const RendererConfigValidation validation = validate_renderer_config(config);
  if (!validation.valid) {
    platform::log::error(std::string{"Renderer init failed: invalid config: "} + validation.reason);
    impl_->set_lifecycle(RendererLifecycleState::Failed);
    return false;
  }

  impl_->set_lifecycle(RendererLifecycleState::Initializing);
  impl_->config = config;
  impl_->reset_flags = impl_->reset_flags_from_config();
  impl_->device_lost = false;
  impl_->frame_active = false;
  impl_->has_pending_resize = false;

  bgfx::Init init{};
  init.type = to_bgfx_renderer_type(config.backend);
  init.resolution.width = config.width;
  init.resolution.height = config.height;
  init.resolution.reset = impl_->reset_flags;

  auto* sdl_window = static_cast<SDL_Window*>(runtime.native_window_handle());
  if (sdl_window == nullptr) {
    platform::log::error("Renderer init failed: runtime window handle was null");
    impl_->set_lifecycle(RendererLifecycleState::Failed);
    return false;
  }

  set_platform_data_from_sdl(init, sdl_window);

  std::ostringstream request_log;
  request_log << "Renderer init requested backend=" << to_string(config.backend)
              << " size=" << config.width << "x" << config.height
              << " vsync=" << (config.vsync ? "on" : "off")
              << " debug=" << (config.debug ? "on" : "off");
  platform::log::info(request_log.str());

  if (!bgfx::init(init)) {
    platform::log::error("Renderer init failed: bgfx::init returned false");
    impl_->set_lifecycle(RendererLifecycleState::Failed);
    return false;
  }
  impl_->bgfx_initialized = true;

  impl_->selected_backend = to_backend(bgfx::getRendererType());
  impl_->backbuffer_width = config.width;
  impl_->backbuffer_height = config.height;

  const std::uint32_t debug_flags = config.debug ? BGFX_DEBUG_TEXT : BGFX_DEBUG_NONE;
  bgfx::setDebug(debug_flags);

  std::ostringstream backend_log;
  backend_log << "Renderer backend selected requested=" << to_string(config.backend)
              << " actual=" << renderer_name(bgfx::getRendererType());
  platform::log::info(backend_log.str());

  if (is_explicit_backend_request(config.backend) && config.backend != impl_->selected_backend) {
    std::ostringstream mismatch_log;
    mismatch_log << "Requested backend " << to_string(config.backend)
                 << " not selected, running on " << to_string(impl_->selected_backend);
    platform::log::warn(mismatch_log.str());
  }

  if (const bgfx::Caps* caps = bgfx::getCaps(); caps != nullptr) {
    std::ostringstream caps_log;
    caps_log << "Renderer caps: maxViews=" << caps->limits.maxViews << ", maxTextureSize=" << caps->limits.maxTextureSize
             << ", homogeneousDepth=" << (caps->homogeneousDepth ? "true" : "false")
             << ", originBottomLeft=" << (caps->originBottomLeft ? "true" : "false");
    platform::log::info(caps_log.str());
  }

  bgfx::setViewRect(kMainView, 0, 0, static_cast<std::uint16_t>(minimum_dimension(config.width)), static_cast<std::uint16_t>(minimum_dimension(config.height)));
  bgfx::setViewClear(kMainView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1f2233ff, 1.0F, 0);

  impl_->set_lifecycle(RendererLifecycleState::Ready);
  return true;
}

void Renderer::shutdown() {
  if (impl_ == nullptr || impl_->lifecycle == RendererLifecycleState::Uninitialized) {
    return;
  }

  impl_->set_lifecycle(RendererLifecycleState::ShuttingDown);
  if (impl_->frame_active) {
    platform::log::warn("Renderer shutdown while a frame is active; ending frame implicitly");
    bgfx::frame();
    impl_->frame_active = false;
  }

  if (impl_->is_initialized()) {
    bgfx::shutdown();
    impl_->bgfx_initialized = false;
  }

  impl_->selected_backend = RendererBackend::Auto;
  impl_->backbuffer_width = 0;
  impl_->backbuffer_height = 0;
  impl_->has_pending_resize = false;
  impl_->set_lifecycle(RendererLifecycleState::Uninitialized);
}

bool Renderer::is_initialized() const noexcept {
  return impl_->lifecycle != RendererLifecycleState::Uninitialized;
}

bool Renderer::is_ready() const noexcept {
  return impl_->lifecycle == RendererLifecycleState::Ready;
}

bool Renderer::can_render() const noexcept {
  return impl_->can_render();
}

RendererLifecycleState Renderer::state() const noexcept {
  return impl_->lifecycle;
}

RendererStatus Renderer::status() const noexcept {
  return impl_->make_status();
}

RendererCaps Renderer::capabilities() const {
  RendererCaps caps{};

  if (impl_->lifecycle != RendererLifecycleState::Ready) {
    return caps;
  }

  caps.renderer_name = renderer_name(bgfx::getRendererType());
  if (const bgfx::Caps* bgfx_caps = bgfx::getCaps(); bgfx_caps != nullptr) {
    caps.supported_features = bgfx_caps->supported;
    caps.max_views = bgfx_caps->limits.maxViews;
    caps.max_texture_size = bgfx_caps->limits.maxTextureSize;
    caps.homogeneous_depth = bgfx_caps->homogeneousDepth;
    caps.origin_bottom_left = bgfx_caps->originBottomLeft;
  }

  return caps;
}

RendererBackend Renderer::backend() const noexcept { return impl_->selected_backend; }

bool Renderer::begin_frame() {
  if (impl_->lifecycle == RendererLifecycleState::Failed) {
    if (impl_->config.allow_automatic_recovery) {
      return try_recover();
    }
    return false;
  }

  if (impl_->lifecycle != RendererLifecycleState::Ready) {
    platform::log::warn(std::string{"begin_frame ignored in state "} + to_string(impl_->lifecycle));
    return false;
  }

  if (impl_->frame_active) {
    platform::log::warn("begin_frame called while frame already active");
    return false;
  }

  impl_->apply_resize_if_needed();
  if (!impl_->can_render()) {
    return false;
  }

  impl_->frame_active = true;
  impl_->frame_begin_time = std::chrono::steady_clock::now();

  if (impl_->config.debug) {
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 0, 0x0f, "render :: backend %s", renderer_name(bgfx::getRendererType()));
  }

  return true;
}

bool Renderer::end_frame() {
  if (impl_->lifecycle != RendererLifecycleState::Ready) {
    return false;
  }

  if (!impl_->frame_active) {
    platform::log::warn("end_frame called without begin_frame");
    return false;
  }

  bgfx::frame();
  impl_->frame_active = false;
  impl_->frame_count += 1;

  const auto now = std::chrono::steady_clock::now();
  impl_->last_frame_time_ms = static_cast<std::uint32_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(now - impl_->frame_begin_time).count());

  if (impl_->config.min_frame_time_ms > 0U && impl_->last_frame_time_ms < impl_->config.min_frame_time_ms) {
    const std::uint32_t sleep_ms = impl_->config.min_frame_time_ms - impl_->last_frame_time_ms;
    platform::PlatformRuntime::sleep_for_milliseconds(sleep_ms);
    impl_->last_frame_time_ms = impl_->config.min_frame_time_ms;
  }

  return true;
}

void Renderer::request_resize(const std::uint32_t width, const std::uint32_t height) {
  if (impl_->lifecycle != RendererLifecycleState::Ready && impl_->lifecycle != RendererLifecycleState::Recovering) {
    return;
  }

  if (width == impl_->backbuffer_width && height == impl_->backbuffer_height) {
    return;
  }

  impl_->queue_resize(width, height);
}

bool Renderer::try_recover() {
  if (impl_->lifecycle != RendererLifecycleState::Failed && impl_->lifecycle != RendererLifecycleState::Recovering) {
    return impl_->lifecycle == RendererLifecycleState::Ready;
  }

  impl_->set_lifecycle(RendererLifecycleState::Recovering);
  impl_->device_lost = true;

  if (impl_->backbuffer_width == 0U || impl_->backbuffer_height == 0U) {
    platform::log::warn("Renderer recovery delayed because backbuffer dimensions are zero");
    impl_->set_lifecycle(RendererLifecycleState::Failed);
    return false;
  }

  bgfx::reset(impl_->backbuffer_width, impl_->backbuffer_height, impl_->reset_flags);
  impl_->device_lost = false;
  impl_->set_lifecycle(RendererLifecycleState::Ready);
  platform::log::info("Renderer recovery succeeded using bgfx::reset");
  return true;
}

void Renderer::resize(const std::uint32_t width, const std::uint32_t height) {
  request_resize(width, height);
  impl_->apply_resize_if_needed();
}

void Renderer::set_debug_enabled(const bool enabled) {
  impl_->config.debug = enabled;
  if (impl_->lifecycle == RendererLifecycleState::Ready) {
    bgfx::setDebug(enabled ? BGFX_DEBUG_TEXT : BGFX_DEBUG_NONE);
  }
}

void Renderer::set_view(const ViewId view, const ViewDescription& desc) {
  if (!impl_->can_render()) {
    return;
  }

  std::uint16_t clear_flags = 0;
  if (desc.clear.clear_color) {
    clear_flags |= BGFX_CLEAR_COLOR;
  }
  if (desc.clear.clear_depth) {
    clear_flags |= BGFX_CLEAR_DEPTH;
  }
  if (desc.clear.clear_stencil) {
    clear_flags |= BGFX_CLEAR_STENCIL;
  }

  bgfx::setViewRect(view.value, desc.rect.x, desc.rect.y, desc.rect.width, desc.rect.height);
  bgfx::setViewClear(view.value, clear_flags, desc.clear.rgba, desc.clear.depth, desc.clear.stencil);
  bgfx::touch(view.value);
}

void Renderer::set_view_transform(
  const ViewId view,
  const std::span<const float, 16> view_transform,
  const std::span<const float, 16> projection) {
  if (!impl_->can_render()) {
    return;
  }
  bgfx::setViewTransform(view.value, view_transform.data(), projection.data());
}

VertexBufferHandle Renderer::create_vertex_buffer(const VertexBufferDescription& desc) {
  if (!impl_->can_render() || desc.data.empty()) {
    return {};
  }

  bgfx::VertexLayout layout;
  layout.begin();
  for (const VertexElement& element : desc.layout.elements) {
    layout.add(
      to_bgfx_attrib(element.attribute),
      element.components,
      to_bgfx_attrib_type(element.type),
      element.normalized,
      element.as_int);
  }
  layout.end();

  const bgfx::Memory* memory = bgfx::copy(desc.data.data(), static_cast<uint32_t>(desc.data.size_bytes()));
  const bgfx::VertexBufferHandle handle = bgfx::createVertexBuffer(memory, layout);
  return VertexBufferHandle{handle.idx};
}

IndexBufferHandle Renderer::create_index_buffer(const IndexBufferDescription& desc) {
  if (!impl_->can_render() || desc.data.empty()) {
    return {};
  }

  const bgfx::Memory* memory = bgfx::copy(desc.data.data(), static_cast<uint32_t>(desc.data.size_bytes()));
  const bgfx::IndexBufferHandle handle = bgfx::createIndexBuffer(memory);
  return IndexBufferHandle{handle.idx};
}

ProgramHandle Renderer::create_program(const ShaderProgramDescription& desc) {
  if (!impl_->can_render()) {
    return {};
  }

  bgfx::ShaderHandle vs = load_shader(desc.vertex_shader_path);
  bgfx::ShaderHandle fs = load_shader(desc.fragment_shader_path);

  if (!bgfx::isValid(vs) || !bgfx::isValid(fs)) {
    if (bgfx::isValid(vs)) {
      bgfx::destroy(vs);
    }
    if (bgfx::isValid(fs)) {
      bgfx::destroy(fs);
    }
    platform::log::error("Program creation failed due to invalid shader handles");
    return {};
  }

  const bgfx::ProgramHandle program = bgfx::createProgram(vs, fs, true);
  if (!bgfx::isValid(program)) {
    platform::log::error("Program creation failed: bgfx returned invalid handle");
    return {};
  }

  if (!desc.debug_name.empty()) {
    bgfx::setName(program, desc.debug_name.c_str());
  }

  return ProgramHandle{program.idx};
}

TextureHandle Renderer::create_solid_color_texture(const SolidColorTextureDescription& desc) {
  if (!impl_->can_render()) {
    return {};
  }

  const std::uint32_t pixel_count = static_cast<std::uint32_t>(desc.texture.width) * static_cast<std::uint32_t>(desc.texture.height);
  const std::uint32_t byte_count = pixel_count * 4U;

  bimg::ImageContainer* image = bimg::imageAlloc(nullptr, bimg::TextureFormat::RGBA8, desc.texture.width, desc.texture.height, 1, false, false);
  if (image == nullptr || image->m_data == nullptr) {
    platform::log::error("Failed to allocate bimg image for solid-color texture");
    return {};
  }

  for (std::uint32_t i = 0; i < byte_count; i += 4U) {
    image->m_data[i + 0U] = desc.r;
    image->m_data[i + 1U] = desc.g;
    image->m_data[i + 2U] = desc.b;
    image->m_data[i + 3U] = desc.a;
  }

  const bgfx::Memory* memory = bgfx::copy(image->m_data, byte_count);
  bimg::imageFree(image);

  const std::uint64_t flags = desc.texture.is_srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE;
  const bgfx::TextureHandle handle = bgfx::createTexture2D(
    desc.texture.width,
    desc.texture.height,
    desc.texture.has_mips,
    1,
    bgfx::TextureFormat::RGBA8,
    flags,
    memory);

  return TextureHandle{handle.idx};
}

void Renderer::destroy_buffer(const VertexBufferHandle handle) {
  if (impl_->lifecycle == RendererLifecycleState::Ready && handle.idx != kInvalidHandle) {
    bgfx::destroy(bgfx::VertexBufferHandle{handle.idx});
  }
}

void Renderer::destroy_buffer(const IndexBufferHandle handle) {
  if (impl_->lifecycle == RendererLifecycleState::Ready && handle.idx != kInvalidHandle) {
    bgfx::destroy(bgfx::IndexBufferHandle{handle.idx});
  }
}

void Renderer::destroy_program(const ProgramHandle handle) {
  if (impl_->lifecycle == RendererLifecycleState::Ready && handle.idx != kInvalidHandle) {
    bgfx::destroy(bgfx::ProgramHandle{handle.idx});
  }
}

void Renderer::destroy_texture(const TextureHandle handle) {
  if (impl_->lifecycle == RendererLifecycleState::Ready && handle.idx != kInvalidHandle) {
    bgfx::destroy(bgfx::TextureHandle{handle.idx});
  }
}

void Renderer::submit(const ViewId view, const MeshSubmission& mesh_submission) {
  if (!impl_->can_render()) {
    return;
  }

  if (mesh_submission.mesh.vertex_buffer.idx == kInvalidHandle || mesh_submission.mesh.index_buffer.idx == kInvalidHandle
      || mesh_submission.program.idx == kInvalidHandle) {
    return;
  }

  bgfx::setTransform(mesh_submission.transform.data());
  bgfx::setVertexBuffer(0, bgfx::VertexBufferHandle{mesh_submission.mesh.vertex_buffer.idx});
  bgfx::setIndexBuffer(bgfx::IndexBufferHandle{mesh_submission.mesh.index_buffer.idx}, 0, mesh_submission.mesh.index_count);

  const std::uint64_t state = mesh_submission.draw_state.state_flags != 0
    ? mesh_submission.draw_state.state_flags
    : static_cast<std::uint64_t>(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
  bgfx::setState(state);
  bgfx::submit(view.value, bgfx::ProgramHandle{mesh_submission.program.idx});
}

}  // namespace render::rendering
