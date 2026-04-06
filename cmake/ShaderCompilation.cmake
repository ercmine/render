function(render_setup_shader_compilation)
  set(RENDER_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/shaders/bin" CACHE PATH "Directory for compiled shader binaries")
  set(RENDER_BGFX_SHADERC "" CACHE FILEPATH "Path to bgfx shaderc executable")

  add_custom_target(render_shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RENDER_SHADER_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND}
      -DRENDER_SOURCE_DIR=${CMAKE_SOURCE_DIR}
      -DRENDER_SHADER_OUTPUT_DIR=${RENDER_SHADER_OUTPUT_DIR}
      -DRENDER_BGFX_SHADERC=${RENDER_BGFX_SHADERC}
      -P "${CMAKE_SOURCE_DIR}/cmake/compile_shaders.cmake"
    COMMENT "Compiling bgfx shaders (if shaderc is available)"
    VERBATIM
  )
endfunction()
