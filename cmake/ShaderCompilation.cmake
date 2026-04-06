function(render_setup_shader_compilation)
  set(RENDER_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/shaders/bin" CACHE PATH "Directory for compiled shader binaries")
  set(RENDER_BGFX_SHADERC "" CACHE FILEPATH "Path to bgfx shaderc executable")
  option(RENDER_REQUIRE_SHADER_COMPILATION "Fail configure/build checks when shader compilation is unavailable" OFF)

  set(shaderc_candidate "${RENDER_BGFX_SHADERC}")
  if(NOT shaderc_candidate AND TARGET shaderc)
    set(shaderc_candidate "$<TARGET_FILE:shaderc>")
  endif()

  add_custom_target(render_shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RENDER_SHADER_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/logs"
    COMMAND ${CMAKE_COMMAND}
      -DRENDER_SOURCE_DIR=${CMAKE_SOURCE_DIR}
      -DRENDER_SHADER_OUTPUT_DIR=${RENDER_SHADER_OUTPUT_DIR}
      -DRENDER_BGFX_SHADERC=${shaderc_candidate}
      -DRENDER_REQUIRE_SHADER_COMPILATION=${RENDER_REQUIRE_SHADER_COMPILATION}
      -DRENDER_SHADER_LOG_FILE=${CMAKE_BINARY_DIR}/logs/shader_compilation.log
      -P "${CMAKE_SOURCE_DIR}/cmake/compile_shaders.cmake"
    COMMENT "Compiling bgfx shaders"
    VERBATIM
  )

  add_custom_target(render_shader_check
    DEPENDS render_shaders
    COMMENT "Validating required shader assets"
  )
endfunction()
