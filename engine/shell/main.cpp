#include "engine/filesystem/filesystem.hpp"
#include "engine/platform/platform_log.hpp"
#include "engine/platform/platform_runtime.hpp"
#include "engine/procgen/procgen.hpp"
#include "engine/render/draw_submission.hpp"
#include "engine/render/lighting.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/render_pass_system.hpp"
#include "engine/render/shader_library.hpp"
#include "engine/render/vfx.hpp"
#include "engine/scene/scene.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

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

bool key_pressed(const render::platform::InputFrame& input, const render::platform::Key key) {
  return input.keyboard.keys[render::platform::to_index(key)].pressed;
}

void apply_debug_mode_hotkeys(const render::platform::InputFrame& input, render::rendering::Renderer& renderer) {
  if (key_pressed(input, render::platform::Key::Num1)) {
    renderer.set_debug_mode(render::rendering::RendererDebugMode::Disabled);
  } else if (key_pressed(input, render::platform::Key::Num2)) {
    renderer.set_debug_mode(render::rendering::RendererDebugMode::Wireframe);
  } else if (key_pressed(input, render::platform::Key::Num3)) {
    renderer.set_debug_mode(render::rendering::RendererDebugMode::Normals);
  } else if (key_pressed(input, render::platform::Key::Num4)) {
    renderer.set_debug_mode(render::rendering::RendererDebugMode::Albedo);
  } else if (key_pressed(input, render::platform::Key::Num5)) {
    renderer.set_debug_mode(render::rendering::RendererDebugMode::Depth);
  } else if (key_pressed(input, render::platform::Key::Num6)) {
    renderer.set_debug_mode(render::rendering::RendererDebugMode::LightVolumes);
  } else if (key_pressed(input, render::platform::Key::Num7)) {
    renderer.set_debug_mode(render::rendering::RendererDebugMode::Overdraw);
  } else if (key_pressed(input, render::platform::Key::Num8)) {
    renderer.set_debug_mode(render::rendering::RendererDebugMode::GpuTiming);
  } else if (key_pressed(input, render::platform::Key::Tab)) {
    renderer.cycle_debug_mode();
  }
}

}  // namespace

int main(int argc, char** argv) {
  render::platform::PlatformRuntime runtime;
  render::platform::RuntimeConfig platform_config{};
  platform_config.app_name = "render-shell";
  platform_config.org_name = "render";
  platform_config.window.title = "render :: Statement 15 ambient vfx shell";
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

  auto upload_cpu_mesh = [&](const render::rendering::CpuMeshData& cpu_mesh) {
    render::rendering::MeshBufferDescription mesh_desc{};
    mesh_desc.layout = cpu_mesh.layout;
    mesh_desc.data = std::span<const std::byte>{cpu_mesh.vertex_data.data(), cpu_mesh.vertex_data.size()};
    mesh_desc.vertex_count = cpu_mesh.vertex_count;
    mesh_desc.usage = render::rendering::BufferUsage::Immutable;

    render::rendering::IndexBufferDescription index_desc{};
    index_desc.data = std::span<const std::byte>{cpu_mesh.index_data.data(), cpu_mesh.index_data.size()};
    index_desc.index_count = cpu_mesh.index_count;
    index_desc.index_type = cpu_mesh.index_type;
    index_desc.usage = render::rendering::BufferUsage::Immutable;

    return std::pair{renderer.create_mesh_buffer(mesh_desc), renderer.create_index_buffer(index_desc)};
  };

  render::rendering::ShaderProgramLibrary shader_library{renderer, filesystem};
  const render::rendering::ShaderProgramId shader_id{.category = "debug", .name = "debug_triangle", .variant = "default"};
  render::rendering::ProgramHandle program = shader_library.load_program(shader_id);
  const render::rendering::ProgramHandle debug_normals = shader_library.load_program(
    render::rendering::ShaderProgramId{.category = "debug", .name = "renderer_debug_normals", .variant = "default"});
  const render::rendering::ProgramHandle debug_albedo = shader_library.load_program(
    render::rendering::ShaderProgramId{.category = "debug", .name = "renderer_debug_albedo", .variant = "default"});
  const render::rendering::ProgramHandle debug_depth = shader_library.load_program(
    render::rendering::ShaderProgramId{.category = "debug", .name = "renderer_debug_depth", .variant = "default"});
  const render::rendering::ProgramHandle debug_overdraw = shader_library.load_program(
    render::rendering::ShaderProgramId{.category = "debug", .name = "renderer_debug_overdraw", .variant = "default"});
  renderer.set_debug_program_overrides(render::rendering::RendererDebugProgramOverrides{
    .normals = debug_normals,
    .albedo = debug_albedo,
    .depth = debug_depth,
    .overdraw = debug_overdraw,
  });

  render::rendering::vfx::VfxSystem vfx_system{renderer, shader_library};
  vfx_system.initialize();

  render::scene::Scene scene;
  const auto root = scene.create_node("root");
  const auto camera_node = scene.create_node("camera");
  scene.set_parent(camera_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
  render::core::Transform camera_local{};
  camera_local.translation = {0.0F, 0.0F, 6.0F};
  scene.set_local_transform(camera_node, camera_local);
  scene.set_camera(camera_node, {});
  scene.set_active_camera(camera_node);

  render::scene::SceneLightingSettings scene_lighting{};
  scene_lighting.fog.enabled = true;
  scene_lighting.fog.density = 0.04F;
  scene_lighting.bloom.enabled = true;
  scene_lighting.bloom.threshold = 0.8F;
  scene_lighting.bloom.intensity = 1.2F;
  scene.set_lighting_settings(scene_lighting);

  const auto sun_node = scene.create_node("sun_directional");
  scene.set_parent(sun_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
  render::scene::LightComponent sun{};
  sun.type = render::scene::LightType::Directional;
  sun.color = {1.0F, 0.95F, 0.85F};
  sun.intensity = 1.8F;
  sun.casts_shadows = true;
  scene.set_light(sun_node, sun);

  const auto primitive_mesh = render::procgen::make_box({0.9F, 0.9F, 0.9F}, 1);

  render::procgen::SplinePath extrude_path{};
  extrude_path.control_points = {{-0.6F, 0.0F, 0.0F}, {-0.2F, 0.5F, 0.3F}, {0.3F, -0.2F, 0.8F}, {0.7F, 0.3F, 1.4F}};
  std::array<render::core::Vec2, 8> extrude_profile{};
  for (std::size_t i = 0; i < extrude_profile.size(); ++i) {
    const float a = static_cast<float>(i) / static_cast<float>(extrude_profile.size()) * render::core::kTwoPi;
    extrude_profile[i] = {std::cos(a) * 0.15F, std::sin(a) * 0.15F};
  }
  const auto extruded_mesh = render::procgen::extrude_profile_along_path(
    extrude_profile, extrude_path, render::procgen::ExtrudeOptions{.path_samples = 24, .cap_ends = true, .closed_profile = true});

  std::array<render::core::Vec2, 5> lathe_profile{{{0.0F, -0.45F}, {0.25F, -0.35F}, {0.3F, 0.0F}, {0.15F, 0.45F}, {0.0F, 0.55F}}};
  const auto lathed_mesh = render::procgen::lathe_profile(lathe_profile, {.radial_segments = 28, .angle_radians = render::core::kTwoPi});

  render::procgen::SegmentDescriptor seg{};
  seg.mesh = render::procgen::make_cylinder(0.12F, 0.65F, 16, 1, true);
  seg.local_from_prev.translation = {0.0F, 0.48F, 0.0F};
  seg.yaw_jitter_radians = 0.4F;
  seg.scale_jitter = 0.15F;
  std::array<render::procgen::SegmentDescriptor, 6> seg_chain{seg, seg, seg, seg, seg, seg};
  const auto segmented_mesh = render::procgen::assemble_segments(seg_chain, {.seed = render::core::Seed::from_string("shell-segment-demo")});

  auto blob_sdf = render::procgen::sdf_smooth_union(render::procgen::sdf_sphere({0.0F, 0.0F, 0.0F}, 0.55F),
                                                    render::procgen::sdf_sphere({0.32F, 0.1F, 0.08F}, 0.36F), 0.2F);
  blob_sdf = render::procgen::sdf_union(blob_sdf, render::procgen::sdf_cylinder({0.0F, -0.45F, 0.0F}, 0.12F, 0.25F));
  render::procgen::ScalarField blob_field(28, 28, 28, 0.06F, {-0.9F, -0.9F, -0.9F});
  render::procgen::rasterize_sdf_to_field(blob_sdf, blob_field);
  const auto sdf_mesh = render::procgen::extract_isosurface(blob_field, 0.0F);

  const std::array demo_meshes{primitive_mesh, extruded_mesh, lathed_mesh, segmented_mesh, sdf_mesh};
  const std::array<render::core::Vec3, 5> demo_positions{{{-2.8F, -0.7F, 0.0F}, {-1.2F, -1.0F, -0.6F}, {0.2F, -0.8F, 0.0F}, {1.9F, -1.2F, -0.4F}, {3.4F, -0.9F, 0.0F}}};
  std::vector<std::pair<render::rendering::MeshBufferHandle, render::rendering::IndexBufferHandle>> demo_gpu_meshes{};
  demo_gpu_meshes.reserve(demo_meshes.size());

  for (std::size_t idx = 0; idx < demo_meshes.size(); ++idx) {
    const auto [mesh_buffer, index_buffer] = upload_cpu_mesh(demo_meshes[idx]);
    demo_gpu_meshes.emplace_back(mesh_buffer, index_buffer);
    const auto node = scene.create_node("proc_mesh_demo");
    scene.set_parent(node, root, render::scene::ReparentPolicy::KeepLocalTransform);
    render::core::Transform t{};
    t.translation = demo_positions[idx];
    scene.set_local_transform(node, t);

    render::scene::RenderableComponent rc{};
    rc.mesh_buffer = mesh_buffer;
    rc.index_buffer = index_buffer;
    rc.index_count = demo_meshes[idx].index_count;
    rc.material.program = program;
    rc.highlighted = (idx == demo_meshes.size() - 1U);
    rc.emissive_color = {0.15F, 0.5F, 1.0F};
    rc.emissive_intensity = (idx == 4U) ? 1.2F : 0.0F;
    scene.set_renderable(node, rc);
  }

  for (std::uint32_t i = 0; i < 10U; ++i) {
    const auto light_node = scene.create_node("biolum_point");
    scene.set_parent(light_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
    render::core::Transform t{};
    t.translation = {static_cast<float>(i) - 5.0F, 1.5F, ((i % 2U) == 0U) ? 1.25F : -1.25F};
    scene.set_local_transform(light_node, t);
    render::scene::LightComponent point{};
    point.type = render::scene::LightType::Point;
    point.color = {0.2F, 0.7F, 1.0F};
    point.intensity = 2.0F;
    point.range = 5.0F;
    scene.set_light(light_node, point);
  }

  const auto spores_node = scene.create_node("vfx_spores");
  scene.set_parent(spores_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
  scene.set_local_transform(spores_node, render::core::Transform{.translation = {-2.2F, 1.4F, 0.5F}, .rotation = {}, .scale = {1.0F, 1.0F, 1.0F}});
  const auto spores_handle = vfx_system.spawn_effect(render::rendering::vfx::VfxSystem::make_spores_definition("hatchery_spores"), *scene.local_transform(spores_node));
  scene.set_vfx_attachment(spores_node, {.effect_handle = spores_handle.value});

  const auto dust_node = scene.create_node("vfx_dust");
  scene.set_parent(dust_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
  scene.set_local_transform(dust_node, render::core::Transform{.translation = {0.0F, 1.8F, 0.0F}, .rotation = {}, .scale = {1.0F, 1.0F, 1.0F}});
  const auto dust_handle = vfx_system.spawn_effect(render::rendering::vfx::VfxSystem::make_dust_definition("market_dust"), *scene.local_transform(dust_node));
  scene.set_vfx_attachment(dust_node, {.effect_handle = dust_handle.value});

  const auto embers_node = scene.create_node("vfx_embers");
  scene.set_parent(embers_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
  scene.set_local_transform(embers_node, render::core::Transform{.translation = {3.4F, -1.5F, 0.0F}, .rotation = {}, .scale = {1.0F, 1.0F, 1.0F}});
  const auto embers_handle = vfx_system.spawn_effect(render::rendering::vfx::VfxSystem::make_embers_definition("forge_embers"), *scene.local_transform(embers_node));
  scene.set_vfx_attachment(embers_node, {.effect_handle = embers_handle.value});

  const auto resin_node = scene.create_node("vfx_resin");
  scene.set_parent(resin_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
  scene.set_local_transform(resin_node, render::core::Transform{.translation = {-3.6F, 2.4F, -1.0F}, .rotation = {}, .scale = {1.0F, 1.0F, 1.0F}});
  const auto resin_handle = vfx_system.spawn_effect(render::rendering::vfx::VfxSystem::make_resin_drips_definition("organic_resin_drips"), *scene.local_transform(resin_node));
  scene.set_vfx_attachment(resin_node, {.effect_handle = resin_handle.value});

  const auto pulse_node = scene.create_node("vfx_pulses");
  scene.set_parent(pulse_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
  scene.set_local_transform(pulse_node, render::core::Transform{.translation = {1.2F, 0.5F, 1.8F}, .rotation = {}, .scale = {1.0F, 1.0F, 1.0F}});
  const auto pulse_handle = vfx_system.spawn_effect(render::rendering::vfx::VfxSystem::make_glow_pulses_definition("crystal_glow_pulse"), *scene.local_transform(pulse_node));
  scene.set_vfx_attachment(pulse_node, {.effect_handle = pulse_handle.value});

  for (const auto& preset : render::rendering::vfx::VfxSystem::make_market_ambience_preset()) {
    const auto market_node = scene.create_node("vfx_market");
    scene.set_parent(market_node, root, render::scene::ReparentPolicy::KeepLocalTransform);
    scene.set_local_transform(market_node, render::core::Transform{.translation = {0.0F, 0.6F, -2.8F}, .rotation = {}, .scale = {1.0F, 1.0F, 1.0F}});
    const auto handle = vfx_system.spawn_effect(preset, *scene.local_transform(market_node));
    scene.set_vfx_attachment(market_node, {.effect_handle = handle.value});
  }

  render::rendering::RenderPassRegistry pass_registry;
  bool logged_graph = false;

  while (!runtime.should_quit()) {
    runtime.begin_frame();
    runtime.pump_events();
    apply_debug_mode_hotkeys(runtime.input(), renderer);

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
    for (const auto& attachment : scene.collect_visible_vfx_attachments()) {
      if (attachment.attachment == nullptr || attachment.world_transform == nullptr) {
        continue;
      }
      vfx_system.set_effect_transform(render::rendering::vfx::EffectHandle{attachment.attachment->effect_handle}, *attachment.world_transform);
    }
    vfx_system.update(1.0F / 60.0F);

    const float aspect_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
    const auto camera_view = scene.build_camera_view(aspect_ratio);
    render::core::Vec3 camera_position{};
    if (camera_view.has_value()) {
      const auto view_matrix = to_array(camera_view->view);
      const auto projection_matrix = to_array(camera_view->projection);
      renderer.set_view_transform(kMainView, std::span<const float, 16>{view_matrix}, std::span<const float, 16>{projection_matrix});
      if (const auto* camera_world = scene.world_transform(camera_view->node); camera_world != nullptr) {
        camera_position = camera_world->translation;
      }
    }

    struct ShellFrameState {
      std::vector<render::rendering::DrawSubmission> submissions{};
      std::vector<render::rendering::DirectionalLightInput> directional_lights{};
      std::vector<render::rendering::PointLightInput> point_lights{};
      std::vector<render::rendering::RenderableLightingInput> lit_inputs{};
      render::rendering::LightingFrameData lighting_frame{};
      render::rendering::SubmissionDiagnostics submission_diagnostics{};
      render::rendering::vfx::EffectDiagnostics vfx_diagnostics{};
      bool built_lighting_frame{false};
    } frame_state;

    const auto visible = scene.collect_visible_renderables();
    const auto visible_lights = scene.collect_visible_lights();
    frame_state.submissions.reserve(visible.size());
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
      frame_state.submissions.push_back(draw);
    }

    for (const auto& visible_light : visible_lights) {
      if (visible_light.light == nullptr || visible_light.world_transform == nullptr) {
        continue;
      }
      if (visible_light.light->type == render::scene::LightType::Directional) {
        const render::core::Vec3 direction = render::core::forward(*visible_light.world_transform) * -1.0F;
        frame_state.directional_lights.push_back(render::rendering::DirectionalLightInput{
          .direction = direction,
          .color = visible_light.light->color,
          .intensity = visible_light.light->intensity,
          .casts_shadows = visible_light.light->casts_shadows,
        });
      } else {
        frame_state.point_lights.push_back(render::rendering::PointLightInput{
          .position = visible_light.world_transform->translation,
          .color = visible_light.light->color,
          .intensity = visible_light.light->intensity,
          .range = visible_light.light->range,
          .casts_shadows = visible_light.light->casts_shadows,
        });
      }
    }

    frame_state.lit_inputs.reserve(visible.size());
    for (const auto& vr : visible) {
      if (vr.world_transform == nullptr || vr.renderable == nullptr) {
        continue;
      }
      frame_state.lit_inputs.push_back(render::rendering::RenderableLightingInput{
        .object_id = vr.node.index,
        .position = vr.world_transform->translation,
        .bounding_radius = 1.0F,
        .highlighted = vr.renderable->highlighted,
      });
    }

    render::rendering::FogSettings fog{};
    fog.enabled = scene.lighting_settings().fog.enabled;
    fog.color = scene.lighting_settings().fog.color;
    fog.density = scene.lighting_settings().fog.density;
    fog.near_distance = scene.lighting_settings().fog.near_distance;
    fog.far_distance = scene.lighting_settings().fog.far_distance;

    render::rendering::BloomSettings bloom{};
    bloom.enabled = scene.lighting_settings().bloom.enabled;
    bloom.threshold = scene.lighting_settings().bloom.threshold;
    bloom.intensity = scene.lighting_settings().bloom.intensity;

    pass_registry.reset();
    std::string graph_error;
    pass_registry.declare_resource({.name = "shadow_map", .kind = render::rendering::RenderResourceKind::ShadowMap, .width = 2048, .height = 2048, .transient = true, .imported = false}, &graph_error);
    pass_registry.declare_resource({.name = "scene_color", .kind = render::rendering::RenderResourceKind::RenderTarget, .width = static_cast<std::uint16_t>(window.width), .height = static_cast<std::uint16_t>(window.height), .transient = true, .imported = false}, &graph_error);
    pass_registry.declare_resource({.name = "bloom_extract", .kind = render::rendering::RenderResourceKind::PostProcessTexture, .width = static_cast<std::uint16_t>(window.width / 2U), .height = static_cast<std::uint16_t>(window.height / 2U), .transient = true, .imported = false}, &graph_error);
    pass_registry.declare_resource({.name = "bloom_blur", .kind = render::rendering::RenderResourceKind::PostProcessTexture, .width = static_cast<std::uint16_t>(window.width / 2U), .height = static_cast<std::uint16_t>(window.height / 2U), .transient = true, .imported = false}, &graph_error);
    pass_registry.declare_resource({.name = "post_color", .kind = render::rendering::RenderResourceKind::PostProcessTexture, .width = static_cast<std::uint16_t>(window.width), .height = static_cast<std::uint16_t>(window.height), .transient = true, .imported = false}, &graph_error);
    pass_registry.declare_resource({.name = "outline_mask", .kind = render::rendering::RenderResourceKind::PostProcessTexture, .width = static_cast<std::uint16_t>(window.width), .height = static_cast<std::uint16_t>(window.height), .transient = true, .imported = false}, &graph_error);
    pass_registry.declare_resource({.name = "ui_overlay", .kind = render::rendering::RenderResourceKind::UiTarget, .width = static_cast<std::uint16_t>(window.width), .height = static_cast<std::uint16_t>(window.height), .transient = true, .imported = false}, &graph_error);
    pass_registry.declare_resource({.name = "debug_overlay", .kind = render::rendering::RenderResourceKind::UiTarget, .width = static_cast<std::uint16_t>(window.width), .height = static_cast<std::uint16_t>(window.height), .transient = true, .imported = false}, &graph_error);
    pass_registry.declare_resource({.name = "backbuffer", .kind = render::rendering::RenderResourceKind::Backbuffer, .width = static_cast<std::uint16_t>(window.width), .height = static_cast<std::uint16_t>(window.height), .transient = false, .imported = true}, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "shadow-pass",
      .depends_on = {},
      .resources = {{.resource = "shadow_map", .access = render::rendering::RenderResourceAccess::Write, .clear_before_use = true}},
      .execute = [&](const render::rendering::RenderPassExecutionContext&) {
        frame_state.lighting_frame = render::rendering::build_forward_plus_frame_data(
          camera_position,
          frame_state.directional_lights,
          frame_state.point_lights,
          frame_state.lit_inputs,
          {},
          fog,
          bloom,
          {},
          {});
        frame_state.built_lighting_frame = true;
      },
      .enabled = {},
    }, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "main-lit-pass",
      .depends_on = {"shadow-pass"},
      .resources = {
        {.resource = "shadow_map", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "scene_color", .access = render::rendering::RenderResourceAccess::Write, .clear_before_use = true},
      },
      .execute = [&](const render::rendering::RenderPassExecutionContext&) {
        frame_state.submission_diagnostics = {};
        const auto batches = render::rendering::build_draw_batches(frame_state.submissions, {}, &frame_state.submission_diagnostics);
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

        auto vfx_submissions = vfx_system.build_frame_submissions(kMainView);
        for (const auto& vfx : vfx_submissions) {
          renderer.submit_instanced(kMainView, vfx.draw, std::span<const float>{vfx.transforms});
        }
        frame_state.vfx_diagnostics = vfx_system.diagnostics();
      },
      .enabled = {},
    }, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "bloom-extract-pass",
      .depends_on = {"main-lit-pass"},
      .resources = {
        {.resource = "scene_color", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "bloom_extract", .access = render::rendering::RenderResourceAccess::Write, .clear_before_use = true},
      },
      .execute = [](const render::rendering::RenderPassExecutionContext&) {},
      .enabled = [bloom](const render::rendering::RenderPassExecutionContext&) { return bloom.enabled; },
    }, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "bloom-blur-pass",
      .depends_on = {"bloom-extract-pass"},
      .resources = {
        {.resource = "bloom_extract", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "bloom_blur", .access = render::rendering::RenderResourceAccess::Write, .clear_before_use = true},
      },
      .execute = [](const render::rendering::RenderPassExecutionContext&) {},
      .enabled = [bloom](const render::rendering::RenderPassExecutionContext&) { return bloom.enabled; },
    }, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "bloom-composite-pass",
      .depends_on = {"main-lit-pass", "bloom-blur-pass"},
      .resources = {
        {.resource = "scene_color", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "bloom_blur", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "post_color", .access = render::rendering::RenderResourceAccess::Write, .clear_before_use = true},
      },
      .execute = [](const render::rendering::RenderPassExecutionContext&) {},
      .enabled = {},
    }, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "outline-pass",
      .depends_on = {"main-lit-pass"},
      .resources = {
        {.resource = "scene_color", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "outline_mask", .access = render::rendering::RenderResourceAccess::Write, .clear_before_use = true},
      },
      .execute = [](const render::rendering::RenderPassExecutionContext&) {},
      .enabled = {},
    }, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "ui-pass",
      .depends_on = {"bloom-composite-pass", "outline-pass"},
      .resources = {
        {.resource = "post_color", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "ui_overlay", .access = render::rendering::RenderResourceAccess::Write, .clear_before_use = true},
      },
      .execute = [](const render::rendering::RenderPassExecutionContext&) {},
      .enabled = {},
    }, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "debug-pass",
      .depends_on = {"ui-pass"},
      .resources = {
        {.resource = "post_color", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "outline_mask", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "ui_overlay", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "debug_overlay", .access = render::rendering::RenderResourceAccess::Write, .clear_before_use = true},
      },
      .execute = [&](const render::rendering::RenderPassExecutionContext&) {
        renderer.set_debug_counters(render::rendering::RendererDebugCounters{
          .visible_directional_lights = static_cast<std::uint32_t>(frame_state.directional_lights.size()),
          .visible_point_lights = static_cast<std::uint32_t>(frame_state.point_lights.size()),
          .submitted_draws = frame_state.submission_diagnostics.submitted_draws,
          .instanced_draws = frame_state.submission_diagnostics.instanced_draws,
          .submitted_instances = frame_state.submission_diagnostics.submitted_instances,
          .vfx_active_effects = frame_state.vfx_diagnostics.total_active_effects,
          .vfx_active_particles = frame_state.vfx_diagnostics.total_active_particles,
          .vfx_draw_calls = frame_state.vfx_diagnostics.draw_calls,
          .vfx_instance_uploads = frame_state.vfx_diagnostics.uploaded_instances,
        });
      },
      .enabled = {},
    }, &graph_error);

    pass_registry.add_pass(render::rendering::RenderPassDefinition{
      .name = "present-pass",
      .depends_on = {"debug-pass"},
      .resources = {
        {.resource = "post_color", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "outline_mask", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "ui_overlay", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "debug_overlay", .access = render::rendering::RenderResourceAccess::Read},
        {.resource = "backbuffer", .access = render::rendering::RenderResourceAccess::Write},
      },
      .execute = [](const render::rendering::RenderPassExecutionContext&) {},
      .enabled = {},
    }, &graph_error);

    if (const auto build_result = pass_registry.build(); !build_result.ok) {
      render::platform::log::error(std::string{"Render pass build failed: "} + build_result.error);
      renderer.end_frame();
      runtime.end_frame();
      continue;
    }

    if (!logged_graph) {
      render::platform::log::info(pass_registry.dump_graph());
      logged_graph = true;
    }

    render::rendering::RenderPassExecutionContext pass_context{};
    pass_context.frame = render::rendering::RenderFrameInfo{
      .frame_index = renderer.status().frame.frame_count,
      .backbuffer_width = static_cast<std::uint16_t>(window.width),
      .backbuffer_height = static_cast<std::uint16_t>(window.height),
      .delta_seconds = 1.0F / 60.0F,
    };
    pass_context.renderer = &renderer;
    pass_context.registry = &pass_registry;
    pass_context.user_data = &frame_state;

    if (!pass_registry.execute(pass_context, &graph_error)) {
      render::platform::log::error(std::string{"Render pass execute failed: "} + graph_error);
    }

    renderer.end_frame();
    runtime.end_frame();
  }

  vfx_system.shutdown();

  renderer.destroy_program(program);
  renderer.destroy_program(debug_normals);
  renderer.destroy_program(debug_albedo);
  renderer.destroy_program(debug_depth);
  renderer.destroy_program(debug_overdraw);
  for (const auto& handles : demo_gpu_meshes) {
    renderer.destroy_buffer(handles.first);
    renderer.destroy_buffer(handles.second);
  }
  renderer.shutdown();
  runtime.shutdown();
  return 0;
}
