include(FetchContent)

set(RENDER_DEPENDENCY_ROOT "${CMAKE_SOURCE_DIR}/third_party" CACHE PATH "Default root folder for vendored dependencies")
set(RENDER_SDL3_ROOT "" CACHE PATH "Optional local path override for SDL3")
set(RENDER_BGFX_ROOT "" CACHE PATH "Optional local path override for bgfx.cmake (contains bgfx/bx/bimg)")
option(RENDER_ALLOW_FETCHCONTENT "Allow FetchContent fallbacks for dependencies" OFF)

function(render_setup_dependencies)
  add_library(render_dependencies INTERFACE)
  add_library(render::dependencies ALIAS render_dependencies)

  render_configure_sdl3()
  render_configure_bgfx_bundle()

  if(TARGET SDL3::SDL3)
    target_link_libraries(render_dependencies INTERFACE SDL3::SDL3)
  else()
    message(FATAL_ERROR "SDL3 integration failed. Set RENDER_SDL3_ROOT/RENDER_DEPENDENCY_ROOT or enable RENDER_ALLOW_FETCHCONTENT.")
  endif()

  if(TARGET bgfx::bgfx)
    target_link_libraries(render_dependencies INTERFACE bgfx::bgfx)
  elseif(TARGET bgfx)
    target_link_libraries(render_dependencies INTERFACE bgfx)
  else()
    message(FATAL_ERROR "bgfx integration failed. Set RENDER_BGFX_ROOT/RENDER_DEPENDENCY_ROOT or enable RENDER_ALLOW_FETCHCONTENT.")
  endif()

  if(TARGET bimg)
    target_link_libraries(render_dependencies INTERFACE bimg)
  elseif(TARGET bimg::bimg)
    target_link_libraries(render_dependencies INTERFACE bimg::bimg)
  endif()

  if(TARGET bx)
    target_link_libraries(render_dependencies INTERFACE bx)
  elseif(TARGET bx::bx)
    target_link_libraries(render_dependencies INTERFACE bx::bx)
  endif()
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

function(render_configure_bgfx_bundle)
  if(TARGET bgfx::bgfx OR TARGET bgfx)
    return()
  endif()

  set(chosen_path "")
  set(source_hint "")

  if(RENDER_BGFX_ROOT)
    set(chosen_path "${RENDER_BGFX_ROOT}")
    set(source_hint "override")
  elseif(EXISTS "${RENDER_DEPENDENCY_ROOT}/bgfx.cmake")
    set(chosen_path "${RENDER_DEPENDENCY_ROOT}/bgfx.cmake")
    set(source_hint "vendored")
  endif()

  if(chosen_path)
    message(STATUS "Dependency bgfx/bx/bimg: ${source_hint} path -> ${chosen_path}")
    if(EXISTS "${chosen_path}/CMakeLists.txt")
      set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
      set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
      add_subdirectory("${chosen_path}" "${CMAKE_BINARY_DIR}/_deps/bgfx-build" EXCLUDE_FROM_ALL)
    else()
      message(FATAL_ERROR "bgfx.cmake path does not contain CMakeLists.txt: ${chosen_path}")
    endif()
    return()
  endif()

  if(RENDER_ALLOW_FETCHCONTENT)
    message(STATUS "Dependency bgfx/bx/bimg: FetchContent fallback enabled")
    FetchContent_Declare(
      bgfx_bundle
      GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
      GIT_TAG 9dce6b7a9dfc0f2357ca1af0c41f9deb0f9f5ef2
    )
    FetchContent_MakeAvailable(bgfx_bundle)
    return()
  endif()

  message(STATUS "Dependency bgfx/bx/bimg: unresolved (set RENDER_BGFX_ROOT or vendor at third_party/bgfx.cmake)")
endfunction()
