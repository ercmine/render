include(FetchContent)

set(RENDER_DEPENDENCY_ROOT "${CMAKE_SOURCE_DIR}/third_party" CACHE PATH "Default root folder for vendored dependencies")
set(RENDER_SDL3_ROOT "" CACHE PATH "Optional local path override for SDL3")
set(RENDER_BGFX_ROOT "" CACHE PATH "Optional local path override for bgfx")
option(RENDER_ALLOW_FETCHCONTENT "Allow FetchContent fallbacks for dependencies" OFF)

function(render_setup_dependencies)
  add_library(render_dependencies INTERFACE)
  add_library(render::dependencies ALIAS render_dependencies)

  render_configure_sdl3()

  if(TARGET SDL3::SDL3)
    target_link_libraries(render_dependencies INTERFACE SDL3::SDL3)
  else()
    message(FATAL_ERROR "SDL3 integration failed. Set RENDER_SDL3_ROOT/RENDER_DEPENDENCY_ROOT or enable RENDER_ALLOW_FETCHCONTENT.")
  endif()

  render_register_dependency(bgfx "${RENDER_BGFX_ROOT}" "${RENDER_DEPENDENCY_ROOT}/bgfx")
endfunction()

function(render_configure_sdl3)
  if(TARGET SDL3::SDL3)
    return()
  endif()

  set(chosen_path "")
  set(source_hint "")

  if(RENDER_SDL3_ROOT)
    set(chosen_path "${RENDER_SDL3_ROOT}")
    set(source_hint "override")
  elseif(EXISTS "${RENDER_DEPENDENCY_ROOT}/SDL3")
    set(chosen_path "${RENDER_DEPENDENCY_ROOT}/SDL3")
    set(source_hint "vendored")
  endif()

  if(chosen_path)
    message(STATUS "Dependency SDL3: ${source_hint} path -> ${chosen_path}")
    if(EXISTS "${chosen_path}/CMakeLists.txt")
      add_subdirectory("${chosen_path}" "${CMAKE_BINARY_DIR}/_deps/sdl3-build" EXCLUDE_FROM_ALL)
    else()
      message(FATAL_ERROR "SDL3 path does not contain CMakeLists.txt: ${chosen_path}")
    endif()
  else()
    find_package(SDL3 QUIET CONFIG)
    if(TARGET SDL3::SDL3)
      message(STATUS "Dependency SDL3: found via system package")
      return()
    endif()

    if(RENDER_ALLOW_FETCHCONTENT)
      message(STATUS "Dependency SDL3: FetchContent fallback enabled")
      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.10
      )
      FetchContent_MakeAvailable(SDL3)
    else()
      message(STATUS "Dependency SDL3: unresolved (set RENDER_SDL3_ROOT or vendor at third_party/SDL3)")
    endif()
  endif()

  if(NOT TARGET SDL3::SDL3)
    if(TARGET SDL3::SDL3-shared)
      add_library(SDL3::SDL3 ALIAS SDL3::SDL3-shared)
    elseif(TARGET SDL3::SDL3-static)
      add_library(SDL3::SDL3 ALIAS SDL3::SDL3-static)
    endif()
  endif()
endfunction()

function(render_register_dependency dep_name override_path vendored_path)
  set(chosen_path "")
  set(source_hint "unresolved")

  if(override_path)
    set(chosen_path "${override_path}")
    set(source_hint "override")
  elseif(EXISTS "${vendored_path}")
    set(chosen_path "${vendored_path}")
    set(source_hint "vendored")
  endif()

  if(chosen_path)
    message(STATUS "Dependency ${dep_name}: ${source_hint} path -> ${chosen_path}")
  else()
    message(STATUS "Dependency ${dep_name}: not configured yet")
  endif()
endfunction()
