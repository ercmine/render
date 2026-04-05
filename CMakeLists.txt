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

render_prevent_in_source_builds()
render_apply_standard_settings()

render_setup_options()
render_setup_dependencies()
render_setup_versioning()

# Placeholder project structure targets.
add_library(render_engine INTERFACE)
add_library(render::engine ALIAS render_engine)
render_apply_project_options(render_engine)
render_apply_warnings(render_engine)
render_mark_target_placeholder(render_engine)

add_library(render_game INTERFACE)
add_library(render::game ALIAS render_game)
target_link_libraries(render_game INTERFACE render::engine)
render_apply_project_options(render_game)
render_apply_warnings(render_game)
render_mark_target_placeholder(render_game)

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
  add_library(render_tests INTERFACE)
  add_library(render::tests ALIAS render_tests)
  target_link_libraries(render_tests INTERFACE render::engine render::game)
  render_apply_project_options(render_tests)
  render_apply_warnings(render_tests)
  render_mark_target_placeholder(render_tests)
endif()

message(STATUS "Configured ${PROJECT_NAME} ${PROJECT_VERSION} for ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Tools: ${RENDER_BUILD_TOOLS}; Tests: ${RENDER_BUILD_TESTS}")
