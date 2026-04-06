if(NOT EXISTS "${RENDER_SOURCE_DIR}/shaders/src")
  message(FATAL_ERROR "Shader source directory missing: ${RENDER_SOURCE_DIR}/shaders/src")
endif()

if(NOT DEFINED RENDER_SHADER_LOG_FILE)
  set(RENDER_SHADER_LOG_FILE "")
endif()

function(render_shader_log line)
  if(RENDER_SHADER_LOG_FILE)
    file(APPEND "${RENDER_SHADER_LOG_FILE}" "${line}\n")
  endif()
endfunction()

if(RENDER_SHADER_LOG_FILE)
  file(WRITE "${RENDER_SHADER_LOG_FILE}" "render shader compilation log\n")
endif()

if(NOT RENDER_BGFX_SHADERC)
  if(RENDER_REQUIRE_SHADER_COMPILATION)
    message(FATAL_ERROR "RENDER_BGFX_SHADERC not set and RENDER_REQUIRE_SHADER_COMPILATION=ON")
  endif()
  message(WARNING "RENDER_BGFX_SHADERC not set; skipping shader compilation. Expected precompiled files in shaders/bin/<backend>.")
  render_shader_log("skipped: shaderc unavailable")
  return()
endif()

if(NOT EXISTS "${RENDER_BGFX_SHADERC}")
  if(RENDER_REQUIRE_SHADER_COMPILATION)
    message(FATAL_ERROR "RENDER_BGFX_SHADERC path does not exist: ${RENDER_BGFX_SHADERC}")
  endif()
  message(WARNING "RENDER_BGFX_SHADERC path does not exist: ${RENDER_BGFX_SHADERC}; shader compilation skipped.")
  render_shader_log("skipped: shaderc path does not exist: ${RENDER_BGFX_SHADERC}")
  return()
endif()

set(SHADER_SOURCE_DIR "${RENDER_SOURCE_DIR}/shaders/src")
set(SHADER_OUTPUT_DIR "${RENDER_SHADER_OUTPUT_DIR}")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")
render_shader_log("shaderc: ${RENDER_BGFX_SHADERC}")
render_shader_log("output: ${SHADER_OUTPUT_DIR}")

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
      render_shader_log("[ok] ${backend}/${stage}_${name}.bin")
    endforeach()
  endforeach()
endforeach()

message(STATUS "Shader compilation complete: ${SHADER_OUTPUT_DIR}")
