cmake_minimum_required(VERSION 3.25)

project(
  render
  VERSION 0.1.0
  DESCRIPTION "Desktop-first custom C++20 engine and game"
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(PreventInSourceBuilds)
include(StandardSettings)
include(ProjectOptions)
include(CompilerWarnings)
include(Dependencies)
include(Versioning)
include(Utils)
include(ShaderCompilation)
include(GNUInstallDirs)

render_prevent_in_source_builds()
render_apply_standard_settings()

render_setup_options()
render_setup_dependencies()
render_setup_versioning()
render_setup_shader_compilation()

if(RENDER_BUILD_TESTS)
  include(CTest)
endif()

add_library(render_core STATIC
  engine/core/types.hpp
  engine/core/math.hpp
  engine/core/math.cpp
  engine/core/transform.hpp
  engine/core/transform.cpp
  engine/core/color.hpp
  engine/core/color.cpp
  engine/core/camera.hpp
  engine/core/camera.cpp
  engine/core/uuid.hpp
  engine/core/uuid.cpp
  engine/core/hash.hpp
  engine/core/hash.cpp
  engine/core/random.hpp
  engine/core/random.cpp
  engine/core/memory.hpp
  engine/core/memory.cpp
  engine/core/threading.hpp
  engine/core/threading.cpp
  engine/core/log.hpp
  engine/core/log.cpp
  engine/core/assert.hpp
  engine/core/assert.cpp
  engine/core/profiling.hpp
  engine/core/profiling.cpp
  engine/core/core.hpp
)
add_library(render::core ALIAS render_core)
target_include_directories(render_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
render_apply_project_options(render_core)
render_apply_warnings(render_core)


add_library(render_serialization STATIC
  engine/serialization/bytes.hpp
  engine/serialization/errors.hpp
  engine/serialization/binary_writer.hpp
  engine/serialization/binary_writer.cpp
  engine/serialization/binary_reader.hpp
  engine/serialization/binary_reader.cpp
  engine/serialization/schema.hpp
  engine/serialization/schema.cpp
  engine/serialization/core_types.hpp
  engine/serialization/core_types.cpp
  engine/serialization/payloads.hpp
  engine/serialization/payloads.cpp
  engine/serialization/serialization.hpp
)
add_library(render::serialization ALIAS render_serialization)
target_include_directories(render_serialization PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(render_serialization PUBLIC render::core)
render_apply_project_options(render_serialization)
render_apply_warnings(render_serialization)


add_library(render_filesystem STATIC
  engine/filesystem/filesystem.hpp
  engine/filesystem/filesystem.cpp
)
add_library(render::filesystem ALIAS render_filesystem)
target_include_directories(render_filesystem PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(render_filesystem PUBLIC render::core)
render_apply_project_options(render_filesystem)
render_apply_warnings(render_filesystem)

add_library(render_platform STATIC
  engine/platform/platform_types.hpp
  engine/platform/platform_runtime.hpp
  engine/platform/platform_log.hpp
  engine/platform/sdl/platform_runtime_sdl.cpp
)
add_library(render::platform ALIAS render_platform)
target_include_directories(render_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(render_platform PUBLIC render::core render::dependencies)
render_apply_project_options(render_platform)
render_apply_warnings(render_platform)

add_library(render_renderer STATIC
  engine/render/renderer.hpp
  engine/render/renderer_types.hpp
  engine/render/buffer_types.hpp
  engine/render/buffer_types.cpp
  engine/render/debug_renderer.hpp
  engine/render/debug_renderer.cpp
  engine/render/draw_submission.hpp
  engine/render/draw_submission.cpp
  engine/render/shader_types.hpp
  engine/render/shader_library.hpp
  engine/render/shader_library.cpp
  engine/render/lighting.hpp
  engine/render/lighting.cpp
  engine/render/texture_types.hpp
  engine/render/render_pass_system.hpp
  engine/render/render_pass_system.cpp
  engine/render/vfx.hpp
  engine/render/vfx.cpp
  engine/render/bgfx/renderer_bgfx.cpp
)
add_library(render::renderer ALIAS render_renderer)
target_include_directories(render_renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(render_renderer PUBLIC render::core render::platform render::dependencies)
render_apply_project_options(render_renderer)
render_apply_warnings(render_renderer)



add_library(render_procgen STATIC
  engine/procgen/procgen.hpp
  engine/procgen/procgen.cpp
)
add_library(render::procgen ALIAS render_procgen)
target_include_directories(render_procgen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(render_procgen PUBLIC render::core render::renderer)
render_apply_project_options(render_procgen)
render_apply_warnings(render_procgen)

add_library(render_scene STATIC
  engine/scene/scene.hpp
  engine/scene/scene.cpp
)
add_library(render::scene ALIAS render_scene)
target_include_directories(render_scene PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(render_scene PUBLIC render::core render::renderer)
render_apply_project_options(render_scene)
render_apply_warnings(render_scene)

add_library(render_engine STATIC
  engine/engine.cpp
)
add_library(render::engine ALIAS render_engine)
target_link_libraries(render_engine PUBLIC render::core render::filesystem render::platform render::renderer render::serialization render::scene render::procgen)
render_apply_project_options(render_engine)
render_apply_warnings(render_engine)

add_library(render_game INTERFACE)
add_library(render::game ALIAS render_game)
target_link_libraries(render_game INTERFACE render::engine)
render_apply_project_options(render_game)
render_apply_warnings(render_game)

add_executable(render_shell engine/shell/main.cpp)
add_executable(render::shell ALIAS render_shell)
target_link_libraries(render_shell PRIVATE render::engine render::filesystem)
add_dependencies(render_shell render_shaders)
render_apply_project_options(render_shell)
render_apply_warnings(render_shell)

if(RENDER_BUILD_TOOLS)
  add_library(render_tools INTERFACE)
  add_library(render::tools ALIAS render_tools)
  target_link_libraries(render_tools INTERFACE render::engine)
  render_apply_project_options(render_tools)
  render_apply_warnings(render_tools)
  render_mark_target_placeholder(render_tools)
endif()

if(RENDER_BUILD_TESTS)
  add_executable(render_platform_types_tests tests/platform/platform_types_tests.cpp)
  target_link_libraries(render_platform_types_tests PRIVATE render::engine)
  render_apply_project_options(render_platform_types_tests)
  render_apply_warnings(render_platform_types_tests)

  add_executable(render_core_runtime_tests tests/core/core_runtime_tests.cpp)
  target_link_libraries(render_core_runtime_tests PRIVATE render::core)
  target_compile_definitions(render_core_runtime_tests PRIVATE RENDER_ENABLE_PROFILING=1)
  render_apply_project_options(render_core_runtime_tests)
  render_apply_warnings(render_core_runtime_tests)


  add_executable(render_serialization_tests tests/serialization/serialization_tests.cpp)
  target_link_libraries(render_serialization_tests PRIVATE render::serialization)
  render_apply_project_options(render_serialization_tests)
  render_apply_warnings(render_serialization_tests)

  add_executable(render_filesystem_tests tests/filesystem/filesystem_tests.cpp)
  target_link_libraries(render_filesystem_tests PRIVATE render::filesystem)
  render_apply_project_options(render_filesystem_tests)
  render_apply_warnings(render_filesystem_tests)

  add_executable(render_headless_validation tests/headless/headless_validation.cpp)
  target_link_libraries(render_headless_validation PRIVATE render::filesystem render::serialization)
  render_apply_project_options(render_headless_validation)
  render_apply_warnings(render_headless_validation)

  add_executable(render_renderer_lifecycle_tests tests/render/renderer_lifecycle_tests.cpp)
  target_link_libraries(render_renderer_lifecycle_tests PRIVATE render::renderer)
  render_apply_project_options(render_renderer_lifecycle_tests)
  render_apply_warnings(render_renderer_lifecycle_tests)

  add_executable(render_shader_pipeline_tests tests/render/shader_pipeline_tests.cpp)
  target_link_libraries(render_shader_pipeline_tests PRIVATE render::renderer render::filesystem)
  render_apply_project_options(render_shader_pipeline_tests)
  render_apply_warnings(render_shader_pipeline_tests)

  add_executable(render_geometry_submission_tests tests/render/geometry_submission_tests.cpp)
  target_link_libraries(render_geometry_submission_tests PRIVATE render::renderer)
  render_apply_project_options(render_geometry_submission_tests)
  render_apply_warnings(render_geometry_submission_tests)

  add_executable(render_lighting_pipeline_tests tests/render/lighting_pipeline_tests.cpp)
  target_link_libraries(render_lighting_pipeline_tests PRIVATE render::renderer)
  render_apply_project_options(render_lighting_pipeline_tests)
  render_apply_warnings(render_lighting_pipeline_tests)

  add_executable(render_debug_renderer_tests tests/render/debug_renderer_tests.cpp)
  target_link_libraries(render_debug_renderer_tests PRIVATE render::renderer)
  render_apply_project_options(render_debug_renderer_tests)
  render_apply_warnings(render_debug_renderer_tests)


  add_executable(render_render_pass_system_tests tests/render/render_pass_system_tests.cpp)
  target_link_libraries(render_render_pass_system_tests PRIVATE render::renderer)
  render_apply_project_options(render_render_pass_system_tests)
  render_apply_warnings(render_render_pass_system_tests)

  add_executable(render_procgen_tests tests/procgen/procgen_tests.cpp)
  target_link_libraries(render_procgen_tests PRIVATE render::procgen)
  render_apply_project_options(render_procgen_tests)
  render_apply_warnings(render_procgen_tests)

  add_executable(render_scene_tests tests/scene/scene_tests.cpp)
  target_link_libraries(render_scene_tests PRIVATE render::scene)
  render_apply_project_options(render_scene_tests)
  render_apply_warnings(render_scene_tests)

  add_executable(render_vfx_system_tests tests/render/vfx_system_tests.cpp)
  target_link_libraries(render_vfx_system_tests PRIVATE render::renderer)
  render_apply_project_options(render_vfx_system_tests)
  render_apply_warnings(render_vfx_system_tests)

  add_test(NAME unit.platform.types COMMAND render_platform_types_tests)
  set_tests_properties(unit.platform.types PROPERTIES LABELS "unit;platform")
  add_test(NAME unit.core.runtime COMMAND render_core_runtime_tests)
  set_tests_properties(unit.core.runtime PROPERTIES LABELS "unit;core")
  add_test(NAME unit.serialization.roundtrip COMMAND render_serialization_tests)
  set_tests_properties(unit.serialization.roundtrip PROPERTIES LABELS "unit;serialization")
  add_test(NAME unit.filesystem.service COMMAND render_filesystem_tests)
  set_tests_properties(unit.filesystem.service PROPERTIES LABELS "unit;filesystem")
  add_test(NAME unit.renderer.lifecycle COMMAND render_renderer_lifecycle_tests)
  set_tests_properties(unit.renderer.lifecycle PROPERTIES LABELS "unit;renderer")
  add_test(NAME unit.renderer.shader_pipeline COMMAND render_shader_pipeline_tests)
  set_tests_properties(unit.renderer.shader_pipeline PROPERTIES LABELS "unit;renderer;shader")
  add_test(NAME unit.renderer.geometry_submission COMMAND render_geometry_submission_tests)
  set_tests_properties(unit.renderer.geometry_submission PROPERTIES LABELS "unit;renderer;geometry")
  add_test(NAME unit.renderer.lighting_pipeline COMMAND render_lighting_pipeline_tests)
  set_tests_properties(unit.renderer.lighting_pipeline PROPERTIES LABELS "unit;renderer;lighting")
  add_test(NAME unit.renderer.debug_views COMMAND render_debug_renderer_tests)
  set_tests_properties(unit.renderer.debug_views PROPERTIES LABELS "unit;renderer;debug")
  add_test(NAME unit.renderer.render_pass_system COMMAND render_render_pass_system_tests)
  set_tests_properties(unit.renderer.render_pass_system PROPERTIES LABELS "unit;renderer;graph")
  add_test(NAME unit.procgen.mesh_toolkit COMMAND render_procgen_tests)
  set_tests_properties(unit.procgen.mesh_toolkit PROPERTIES LABELS "unit;procgen")
  add_test(NAME unit.scene.runtime COMMAND render_scene_tests)
  set_tests_properties(unit.scene.runtime PROPERTIES LABELS "unit;scene")
  add_test(NAME unit.renderer.vfx_system COMMAND render_vfx_system_tests)
  set_tests_properties(unit.renderer.vfx_system PROPERTIES LABELS "unit;renderer;vfx")
  add_test(NAME headless.smoke.startup COMMAND render_headless_validation)
  set_tests_properties(headless.smoke.startup PROPERTIES LABELS "headless;smoke")

  add_custom_target(render_test_unit
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --label-regex unit
    DEPENDS render_platform_types_tests render_core_runtime_tests render_serialization_tests render_filesystem_tests render_renderer_lifecycle_tests render_shader_pipeline_tests render_geometry_submission_tests render_lighting_pipeline_tests render_debug_renderer_tests render_procgen_tests render_scene_tests render_vfx_system_tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Running render unit tests"
  )

  add_custom_target(render_test_headless
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --label-regex headless
    DEPENDS render_headless_validation
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Running render headless smoke validation"
  )
endif()

set(RENDER_PACKAGE_STAGE_DIR "${CMAKE_BINARY_DIR}/package/stage")
set(RENDER_PACKAGE_BIN_SUBDIR "${CMAKE_INSTALL_BINDIR}")
set(RENDER_PACKAGE_SHADER_RELATIVE_DIR "shaders/bin")

install(TARGETS render_shell RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders/bin/" DESTINATION "shaders/bin" OPTIONAL)
install(DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders/metadata/" DESTINATION "shaders/metadata" OPTIONAL)

add_custom_target(render_package_stage
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${RENDER_PACKAGE_STAGE_DIR}"
  DEPENDS render_shell render_shader_check
  COMMENT "Staging render package layout"
)
if(CMAKE_CONFIGURATION_TYPES)
  add_custom_command(TARGET render_package_stage
    COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${RENDER_PACKAGE_STAGE_DIR}" --config $<CONFIG>
  )
else()
  add_custom_command(TARGET render_package_stage
    COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${RENDER_PACKAGE_STAGE_DIR}"
  )
endif()

add_custom_target(render_package_validate
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/logs"
  COMMAND ${CMAKE_COMMAND}
    -DRENDER_PACKAGE_STAGE_DIR=${RENDER_PACKAGE_STAGE_DIR}
    -DRENDER_PACKAGE_BIN_SUBDIR=${RENDER_PACKAGE_BIN_SUBDIR}
    -DRENDER_PACKAGE_SHADER_RELATIVE_DIR=${RENDER_PACKAGE_SHADER_RELATIVE_DIR}
    -DRENDER_PACKAGE_LOG_FILE=${CMAKE_BINARY_DIR}/logs/package_validation.log
    -P "${CMAKE_SOURCE_DIR}/cmake/validate_package_layout.cmake"
  DEPENDS render_package_stage
  COMMENT "Validating package stage and archive creation"
  VERBATIM
)

add_custom_target(render_validate_all
  DEPENDS render_shader_check render_package_validate
  COMMENT "Build/render validation umbrella target"
)
if(RENDER_BUILD_TESTS)
  add_dependencies(render_validate_all render_test_unit render_test_headless)
endif()

message(STATUS "Configured ${PROJECT_NAME} ${PROJECT_VERSION} for ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Tools: ${RENDER_BUILD_TOOLS}; Tests: ${RENDER_BUILD_TESTS}")
