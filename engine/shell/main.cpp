#include "engine/filesystem/filesystem.hpp"
#include "engine/platform/platform_log.hpp"
#include "engine/platform/platform_runtime.hpp"
#include "engine/render/draw_submission.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/shader_library.hpp"
#include "engine/scene/scene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
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
    if (arg == "--backend=noop") return render::rendering::RendererBackend::Noop;
    if (arg == "--backend=d3d11") return render::rendering::RendererBackend::Direct3D11;
    if (arg == "--backend=d3d12") return render::rendering::RendererBackend::Direct3D12;
    if (arg == "--backend=metal") return render::rendering::RendererBackend::Metal;
    if (arg == "--backend=vulkan") return render::rendering::RendererBackend::Vulkan;
    if (arg == "--backend=opengl") return render::rendering::RendererBackend::OpenGL;
  }
  return render::rendering::RendererBackend::Auto;
}

std::array<float, 16> to_array(const render::core::Mat4& matrix) { return matrix.m; }

}  // namespace

int main(int argc, char** argv) {
  render::platform::PlatformRuntime runtime;
  render::platform::RuntimeConfig platform_config{};
  platform_config.app_name = "render-shell";
  platform_config.org_name = "render";
  platform_config.window.title = "render :: Statement 12 geometry shell";
  platform_config.window.width = 1280;
  platform_config.window.height = 720;
  platform_config.window.resizable = true;
  if (!runtime.initialize(platform_config)) return 1;

  render::filesystem::FileSystemService filesystem;
  render::filesystem::StorageConfig storage_config{};
  storage_config.app_name = platform_config.app_name;
  storage_config.org_name = platform_config.org_name;
  storage_config.platform_base_path = runtime.paths().base_path;
  storage_config.platform_pref_path = runtime.paths().pref_path;
  if (!runtime.paths().temp_path.empty()) storage_config.platform_temp_path = runtime.paths().temp_path;
  if (!filesystem.initialize(storage_config)) return 1;

  const render::platform::WindowState& initial_window = runtime.window_state();
  render::rendering::Renderer renderer;
  render::rendering::RendererConfig renderer_config{};
  renderer_config.backend = parse_renderer_backend_from_args(argc, argv);
  renderer_config.width = initial_window.width;
  renderer_config.height = initial_window.height;
  renderer_config.debug = true;
  if (!renderer.initialize(renderer_config, runtime)) return 1;

  const std::vector<PosColorVertex> vertices = {
    {-0.5F, -0.5F, 0.0F, 0xff4040ff}, {0.5F, -0.5F, 0.0F, 0xff40ff40}, {0.0F, 0.5F, 0.0F, 0xffff4040}};
  const std::vector<std::uint16_t> indices = {0, 1, 2};

  render::rendering::MeshBufferDescription mesh_desc{};
  mesh_desc.layout.stride = static_cast<std::uint16_t>(sizeof(PosColorVertex));
  mesh_desc.layout.elements = {
    {render::rendering::VertexAttribute::Position, 0, 3, render::rendering::AttributeType::Float, false, false},
    {render::rendering::VertexAttribute::Color0, 12, 4, render::rendering::AttributeType::Uint8, true, false},
  };
  mesh_desc.data = std::as_bytes(std::span{vertices});
  mesh_desc.vertex_count = static_cast<std::uint32_t>(vertices.size());
  mesh_desc.usage = render::rendering::BufferUsage::Immutable;

  render::rendering::IndexBufferDescription index_desc{};
  index_desc.data = std::as_bytes(std::span{indices});
  index_desc.index_count = static_cast<std::uint32_t>(indices.size());

  const auto mesh_buffer = renderer.create_mesh_buffer(mesh_desc);
  const auto index_buffer = renderer.create_index_buffer(index_desc);

  render::rendering::ShaderProgramLibrary shader_library{renderer, filesystem};
  const render::rendering::ShaderProgramId shader_id{.category = "debug", .name = "debug_triangle", .variant = "default"};
  render::rendering::ProgramHandle program = shader_library.load_program(shader_id);

  render::scene::Scene scene;
  const auto root = scene.create_node("root");
  const auto camera_node = scene.create_node("camera");
  scene.set_parent(camera_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
  render::core::Transform camera_local{};
  camera_local.translation = {0.0F, 0.0F, 6.0F};
  scene.set_local_transform(camera_node, camera_local);
  scene.set_camera(camera_node, {});
  scene.set_active_camera(camera_node);

  constexpr std::uint32_t kInstanceRows = 12;
  std::vector<render::scene::SceneNodeId> nodes;
  nodes.reserve(kInstanceRows * kInstanceRows);
  for (std::uint32_t y = 0; y < kInstanceRows; ++y) {
    for (std::uint32_t x = 0; x < kInstanceRows; ++x) {
      const auto node = scene.create_node("tri_instance");
      scene.set_parent(node, root, render::scene::ReparentPolicy::KeepLocalTransform);
      render::core::Transform t{};
      t.translation = {static_cast<float>(x) - 6.0F, static_cast<float>(y) - 6.0F, 0.0F};
      t.scale = {0.15F, 0.15F, 0.15F};
      scene.set_local_transform(node, t);
      render::scene::RenderableComponent rc{};
      rc.mesh_buffer = mesh_buffer;
      rc.index_buffer = index_buffer;
      rc.index_count = static_cast<std::uint32_t>(indices.size());
      rc.material.program = program;
      scene.set_renderable(node, rc);
      nodes.push_back(node);
    }
  }

  while (!runtime.should_quit()) {
    runtime.begin_frame();
    runtime.pump_events();

    const auto& window = runtime.window_state();
    if (window.resized_this_frame) renderer.request_resize(window.width, window.height);
    if (!renderer.begin_frame()) {
      runtime.end_frame();
      continue;
    }

    render::rendering::ViewDescription view{};
    view.rect.width = static_cast<std::uint16_t>(window.width);
    view.rect.height = static_cast<std::uint16_t>(window.height);
    view.clear.rgba = 0x1f2233ff;
    constexpr render::rendering::ViewId kMainView{0};
    renderer.set_view(kMainView, view);

    shader_library.reload_if_stale(shader_id, program);
    scene.update_world_transforms();

    const float aspect_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
    const auto camera_view = scene.build_camera_view(aspect_ratio);
    if (camera_view.has_value()) {
      const auto view_matrix = to_array(camera_view->view);
      const auto projection_matrix = to_array(camera_view->projection);
      renderer.set_view_transform(kMainView, std::span<const float, 16>{view_matrix}, std::span<const float, 16>{projection_matrix});
    }

    std::vector<render::rendering::DrawSubmission> submissions;
    const auto visible = scene.collect_visible_renderables();
    submissions.reserve(visible.size());
    for (const auto& vr : visible) {
      if (vr.renderable == nullptr || vr.world_transform == nullptr || !vr.renderable->material.valid()) continue;
      const auto world = to_array(vr.world_transform->to_matrix());
      render::rendering::DrawSubmission draw{};
      draw.view = kMainView;
      draw.mesh.vertex_buffer = vr.renderable->mesh_buffer;
      draw.mesh.index_buffer = vr.renderable->index_buffer;
      draw.mesh.index_count = vr.renderable->index_count;
      draw.material = vr.renderable->material;
      draw.draw_state = vr.renderable->draw_state;
      draw.transform = world;
      draw.sort_key = static_cast<std::uint32_t>(vr.node.index);
      submissions.push_back(draw);
    }

    render::rendering::SubmissionDiagnostics diagnostics{};
    const auto batches = render::rendering::build_draw_batches(submissions, {}, &diagnostics);
    for (const auto& batch : batches) {
      if (batch.mode == render::rendering::SubmissionMode::Instanced) {
        render::rendering::DrawSubmission base{};
        base.view = kMainView;
        base.mesh = batch.mesh;
        base.material = batch.material;
        base.draw_state = batch.draw_state;
        base.transform = {};
        renderer.submit_instanced(kMainView, base, std::span<const float>{batch.transforms});
      } else if (!batch.unique_draws.empty()) {
        renderer.submit(kMainView, batch.unique_draws.front());
      }
    }

    renderer.end_frame();
    runtime.end_frame();
  }

  renderer.destroy_program(program);
  renderer.destroy_buffer(mesh_buffer);
  renderer.destroy_buffer(index_buffer);
  renderer.shutdown();
  runtime.shutdown();
  return 0;
}
