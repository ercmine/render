#include "engine/platform/platform_log.hpp"
#include "engine/platform/platform_runtime.hpp"
#include "engine/render/renderer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct PosColorVertex {
  float x;
  float y;
  float z;
  std::uint32_t abgr;
};

render::rendering::RendererBackend parse_renderer_backend_from_args(const int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "--backend=noop") {
      return render::rendering::RendererBackend::Noop;
    }
    if (arg == "--backend=d3d11") {
      return render::rendering::RendererBackend::Direct3D11;
    }
    if (arg == "--backend=d3d12") {
      return render::rendering::RendererBackend::Direct3D12;
    }
    if (arg == "--backend=metal") {
      return render::rendering::RendererBackend::Metal;
    }
    if (arg == "--backend=vulkan") {
      return render::rendering::RendererBackend::Vulkan;
    }
    if (arg == "--backend=opengl") {
      return render::rendering::RendererBackend::OpenGL;
    }
  }

  return render::rendering::RendererBackend::Auto;
}

std::filesystem::path shader_backend_directory(const render::rendering::RendererBackend backend) {
  switch (backend) {
    case render::rendering::RendererBackend::Direct3D11:
    case render::rendering::RendererBackend::Direct3D12: return "dx11";
    case render::rendering::RendererBackend::Metal: return "metal";
    case render::rendering::RendererBackend::Vulkan: return "spirv";
    case render::rendering::RendererBackend::OpenGL: return "glsl";
    default: return "spirv";
  }
}

std::array<float, 16> identity_matrix() {
  return {
    1.0F, 0.0F, 0.0F, 0.0F,
    0.0F, 1.0F, 0.0F, 0.0F,
    0.0F, 0.0F, 1.0F, 0.0F,
    0.0F, 0.0F, 0.0F, 1.0F,
  };
}

}  // namespace

int main(int argc, char** argv) {
  render::platform::PlatformRuntime runtime;

  render::platform::RuntimeConfig platform_config{};
  platform_config.app_name = "render-shell";
  platform_config.org_name = "render";
  platform_config.window.title = "render :: Statement 4 shell";
  platform_config.window.width = 1280;
  platform_config.window.height = 720;
  platform_config.window.resizable = true;

  if (!runtime.initialize(platform_config)) {
    render::platform::log::error("Platform runtime failed to initialize");
    return 1;
  }

  const render::platform::WindowState& initial_window = runtime.window_state();

  render::rendering::Renderer renderer;
  render::rendering::RendererConfig renderer_config{};
  renderer_config.backend = parse_renderer_backend_from_args(argc, argv);
  renderer_config.width = initial_window.width;
  renderer_config.height = initial_window.height;
  renderer_config.debug = true;
  renderer_config.vsync = true;

  if (!renderer.initialize(renderer_config, runtime)) {
    render::platform::log::error("Renderer failed to initialize");
    runtime.shutdown();
    return 1;
  }

  const std::vector<PosColorVertex> vertices = {
    {-0.6F, -0.4F, 0.0F, 0xff0000ff},
    {0.6F, -0.4F, 0.0F, 0xff00ff00},
    {0.0F, 0.6F, 0.0F, 0xffff0000},
  };
  const std::vector<std::uint16_t> indices = {0, 1, 2};

  const render::rendering::VertexBufferDescription vertex_buffer_desc{
    .layout = render::rendering::VertexLayoutDescription{
      .elements = {
        {render::rendering::VertexAttribute::Position, 3, render::rendering::AttributeType::Float, false, false},
        {render::rendering::VertexAttribute::Color0, 4, render::rendering::AttributeType::Uint8, true, false},
      },
    },
    .data = std::as_bytes(std::span{vertices}),
  };

  const render::rendering::IndexBufferDescription index_buffer_desc{.data = std::span{indices}};

  const render::rendering::VertexBufferHandle vertex_buffer = renderer.create_vertex_buffer(vertex_buffer_desc);
  const render::rendering::IndexBufferHandle index_buffer = renderer.create_index_buffer(index_buffer_desc);

  const std::filesystem::path shader_root =
    std::filesystem::path{runtime.paths().base_path} / "shaders/bin" / shader_backend_directory(renderer.backend());
  const render::rendering::ShaderProgramDescription program_desc{
    .vertex_shader_path = shader_root / "vs_statement4.bin",
    .fragment_shader_path = shader_root / "fs_statement4.bin",
    .debug_name = "statement4_program",
  };
  const render::rendering::ProgramHandle program = renderer.create_program(program_desc);

  const auto identity = identity_matrix();

  while (!runtime.should_quit()) {
    runtime.begin_frame();
    runtime.pump_events();

    const render::platform::WindowState& window = runtime.window_state();
    if (window.resized_this_frame && window.width > 0 && window.height > 0) {
      renderer.resize(window.width, window.height);
    }

    renderer.begin_frame();

    render::rendering::ViewDescription view{};
    view.rect.width = static_cast<std::uint16_t>(window.width);
    view.rect.height = static_cast<std::uint16_t>(window.height);
    view.clear.rgba = 0x1f2233ff;
    view.clear.clear_color = true;
    view.clear.clear_depth = true;
    view.clear.clear_stencil = false;

    constexpr render::rendering::ViewId kMainView{0};
    renderer.set_view(kMainView, view);
    renderer.set_view_transform(kMainView, std::span<const float, 16>{identity}, std::span<const float, 16>{identity});

    if (program.idx != render::rendering::kInvalidHandle) {
      render::rendering::MeshSubmission submission{};
      submission.mesh = render::rendering::MeshHandle{
        .vertex_buffer = vertex_buffer,
        .index_buffer = index_buffer,
        .index_count = static_cast<std::uint32_t>(indices.size()),
      };
      submission.program = program;
      submission.transform = std::span<const float, 16>{identity};
      renderer.submit(kMainView, submission);
    }

    renderer.end_frame();
    runtime.end_frame();

    render::platform::PlatformRuntime::sleep_for_milliseconds(1);
  }

  renderer.destroy_program(program);
  renderer.destroy_buffer(vertex_buffer);
  renderer.destroy_buffer(index_buffer);
  renderer.shutdown();
  runtime.shutdown();

  return 0;
}
