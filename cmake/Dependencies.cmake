include(FetchContent)

# Dependency bootstrap strategy:
# 1) Prefer explicit local paths passed by users (presets/environment/cache variables).
# 2) Support vendored source trees under third_party/.
# 3) Allow future opt-in FetchContent fallbacks when needed.

set(RENDER_DEPENDENCY_ROOT "${CMAKE_SOURCE_DIR}/third_party" CACHE PATH "Default root folder for vendored dependencies")
set(RENDER_SDL3_ROOT "" CACHE PATH "Optional local path override for SDL3")
set(RENDER_BGFX_ROOT "" CACHE PATH "Optional local path override for bgfx")
option(RENDER_ALLOW_FETCHCONTENT "Allow FetchContent fallbacks for dependencies" OFF)

function(render_setup_dependencies)
  add_library(render_dependencies INTERFACE)
  add_library(render::dependencies ALIAS render_dependencies)

  render_register_dependency(SDL3 "${RENDER_SDL3_ROOT}" "${RENDER_DEPENDENCY_ROOT}/SDL3")
  render_register_dependency(bgfx "${RENDER_BGFX_ROOT}" "${RENDER_DEPENDENCY_ROOT}/bgfx")
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
    message(STATUS "Dependency ${dep_name}: not configured yet (expected for early scaffolding)")
    if(RENDER_ALLOW_FETCHCONTENT)
      message(STATUS "Dependency ${dep_name}: FetchContent fallback is enabled but not implemented yet")
    endif()
  endif()
endfunction()
