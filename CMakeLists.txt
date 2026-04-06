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

render_prevent_in_source_builds()
render_apply_standard_settings()

render_setup_options()
render_setup_dependencies()
render_setup_versioning()
render_setup_shader_compilation()

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
  engine/render/shader_types.hpp
  engine/render/texture_types.hpp
  engine/render/bgfx/renderer_bgfx.cpp
)
add_library(render::renderer ALIAS render_renderer)
target_include_directories(render_renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(render_renderer PUBLIC render::core render::platform render::dependencies)
render_apply_project_options(render_renderer)
render_apply_warnings(render_renderer)

add_library(render_engine STATIC
  engine/engine.cpp
)
add_library(render::engine ALIAS render_engine)
target_link_libraries(render_engine PUBLIC render::core render::platform render::renderer)
render_apply_project_options(render_engine)
render_apply_warnings(render_engine)

add_library(render_game INTERFACE)
add_library(render::game ALIAS render_game)
target_link_libraries(render_game INTERFACE render::engine)
render_apply_project_options(render_game)
render_apply_warnings(render_game)

add_executable(render_shell engine/shell/main.cpp)
add_executable(render::shell ALIAS render_shell)
target_link_libraries(render_shell PRIVATE render::engine)
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
  include(CTest)

  add_executable(render_platform_types_tests tests/platform/platform_types_tests.cpp)
  target_link_libraries(render_platform_types_tests PRIVATE render::engine)
  render_apply_project_options(render_platform_types_tests)
  render_apply_warnings(render_platform_types_tests)

  add_executable(render_core_runtime_tests tests/core/core_runtime_tests.cpp)
  target_link_libraries(render_core_runtime_tests PRIVATE render::core)
  target_compile_definitions(render_core_runtime_tests PRIVATE RENDER_ENABLE_PROFILING=1)
  render_apply_project_options(render_core_runtime_tests)
  render_apply_warnings(render_core_runtime_tests)

  add_test(NAME render_platform_types_tests COMMAND render_platform_types_tests)
  add_test(NAME render_core_runtime_tests COMMAND render_core_runtime_tests)
endif()

message(STATUS "Configured ${PROJECT_NAME} ${PROJECT_VERSION} for ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Tools: ${RENDER_BUILD_TOOLS}; Tests: ${RENDER_BUILD_TESTS}")
