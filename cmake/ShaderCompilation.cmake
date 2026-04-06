function(render_setup_shader_compilation)
  set(RENDER_SHADER_OUTPUT_ROOT "${CMAKE_BINARY_DIR}/bin/shaders" CACHE PATH "Root directory for compiled shader artifacts")
  set(RENDER_SHADER_MANIFEST "${CMAKE_SOURCE_DIR}/shaders/shaders.cmake" CACHE FILEPATH "Shader manifest file")
  set(RENDER_BGFX_SHADERC "" CACHE FILEPATH "Path to bgfx shaderc executable")
  set(RENDER_SHADER_BACKENDS "" CACHE STRING "Semicolon-separated backend list override (dx11;spirv;glsl;metal)")
  option(RENDER_REQUIRE_SHADER_COMPILATION "Fail configure/build checks when shader compilation is unavailable" OFF)

  set(shaderc_candidate "${RENDER_BGFX_SHADERC}")
  if(NOT shaderc_candidate AND TARGET shaderc)
    set(shaderc_candidate "$<TARGET_FILE:shaderc>")
  endif()

  if(RENDER_SHADER_BACKENDS)
    set(backend_list "${RENDER_SHADER_BACKENDS}")
  elseif(WIN32)
    set(backend_list "dx11")
  elseif(APPLE)
    set(backend_list "metal")
  else()
    set(backend_list "spirv;glsl")
  endif()

  add_custom_target(render_shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RENDER_SHADER_OUTPUT_ROOT}/bin"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RENDER_SHADER_OUTPUT_ROOT}/metadata"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/logs"
    COMMAND ${CMAKE_COMMAND}
      -DRENDER_SOURCE_DIR=${CMAKE_SOURCE_DIR}
      -DRENDER_SHADER_OUTPUT_ROOT=${RENDER_SHADER_OUTPUT_ROOT}
      -DRENDER_SHADER_MANIFEST=${RENDER_SHADER_MANIFEST}
      -DRENDER_BGFX_SHADERC=${shaderc_candidate}
      -DRENDER_REQUIRE_SHADER_COMPILATION=${RENDER_REQUIRE_SHADER_COMPILATION}
      -DRENDER_SHADER_BACKENDS=${backend_list}
      -DRENDER_SHADER_LOG_FILE=${CMAKE_BINARY_DIR}/logs/shader_compilation.log
      -P "${CMAKE_SOURCE_DIR}/cmake/compile_shaders.cmake"
    COMMENT "Compiling render shader pipeline assets"
    VERBATIM
  )

  add_custom_target(render_shader_check
    DEPENDS render_shaders
    COMMENT "Validating required shader assets"
  )
endfunction()
