if(NOT EXISTS "${RENDER_SOURCE_DIR}/shaders/src")
  message(FATAL_ERROR "Shader source directory missing: ${RENDER_SOURCE_DIR}/shaders/src")
endif()

if(NOT RENDER_BGFX_SHADERC)
  message(WARNING "RENDER_BGFX_SHADERC not set; skipping shader compilation. Expected precompiled files in shaders/bin/<backend>.")
  return()
endif()

if(NOT EXISTS "${RENDER_BGFX_SHADERC}")
  message(FATAL_ERROR "RENDER_BGFX_SHADERC path does not exist: ${RENDER_BGFX_SHADERC}")
endif()

set(SHADER_SOURCE_DIR "${RENDER_SOURCE_DIR}/shaders/src")
set(SHADER_OUTPUT_DIR "${RENDER_SHADER_OUTPUT_DIR}")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

set(RENDER_BACKENDS spirv glsl metal dx11)
set(RENDER_SHADER_STAGES vs fs)
set(RENDER_SHADER_NAMES statement4)

foreach(backend IN LISTS RENDER_BACKENDS)
  file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}/${backend}")

  foreach(stage IN LISTS RENDER_SHADER_STAGES)
    foreach(name IN LISTS RENDER_SHADER_NAMES)
      set(in_file "${SHADER_SOURCE_DIR}/${stage}_${name}.sc")
      set(out_file "${SHADER_OUTPUT_DIR}/${backend}/${stage}_${name}.bin")

      if(stage STREQUAL "vs")
        set(shader_type "vertex")
      else()
        set(shader_type "fragment")
      endif()

      if(backend STREQUAL "spirv")
        set(platform linux)
        set(profile spirv)
      elseif(backend STREQUAL "glsl")
        set(platform linux)
        set(profile 330)
      elseif(backend STREQUAL "metal")
        set(platform osx)
        set(profile metal)
      elseif(backend STREQUAL "dx11")
        set(platform windows)
        if(stage STREQUAL "vs")
          set(profile vs_5_0)
        else()
          set(profile ps_5_0)
        endif()
      endif()

      execute_process(
        COMMAND "${RENDER_BGFX_SHADERC}" --type ${shader_type} --platform ${platform} --profile ${profile}
          --include-dir "${SHADER_SOURCE_DIR}" -f "${in_file}" -o "${out_file}"
        RESULT_VARIABLE shader_result
        OUTPUT_VARIABLE shader_stdout
        ERROR_VARIABLE shader_stderr
      )

      if(NOT shader_result EQUAL 0)
        message(FATAL_ERROR "Failed to compile ${in_file} for ${backend}:\n${shader_stdout}\n${shader_stderr}")
      endif()
    endforeach()
  endforeach()
endforeach()

message(STATUS "Shader compilation complete: ${SHADER_OUTPUT_DIR}")
