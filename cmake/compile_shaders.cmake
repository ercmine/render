if(NOT EXISTS "${RENDER_SOURCE_DIR}/shaders")
  message(FATAL_ERROR "Shader source directory missing: ${RENDER_SOURCE_DIR}/shaders")
endif()

if(NOT EXISTS "${RENDER_SHADER_MANIFEST}")
  message(FATAL_ERROR "Shader manifest missing: ${RENDER_SHADER_MANIFEST}")
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
  message(WARNING "RENDER_BGFX_SHADERC not set; skipping shader compilation. Expected staged files in ${RENDER_SHADER_OUTPUT_ROOT}.")
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

include("${RENDER_SHADER_MANIFEST}")

set(SHADER_ROOT "${RENDER_SOURCE_DIR}/shaders")
set(SHADER_SOURCE_ROOT "${SHADER_ROOT}")
set(SHADER_OUTPUT_BIN_ROOT "${RENDER_SHADER_OUTPUT_ROOT}/bin")
set(SHADER_OUTPUT_META_ROOT "${RENDER_SHADER_OUTPUT_ROOT}/metadata")

if(NOT RENDER_SHADER_BACKENDS)
  message(FATAL_ERROR "RENDER_SHADER_BACKENDS is empty")
endif()

set(RENDER_SHADER_STAGES vs fs cs)

function(render_backend_profile backend stage out_platform_var out_profile_var)
  if("${backend}" STREQUAL "spirv")
    set(platform linux PARENT_SCOPE)
    if("${stage}" STREQUAL "cs")
      set(profile spirv15-12 PARENT_SCOPE)
    else()
      set(profile spirv PARENT_SCOPE)
    endif()
  elseif("${backend}" STREQUAL "glsl")
    set(platform linux PARENT_SCOPE)
    if("${stage}" STREQUAL "cs")
      set(profile 430 PARENT_SCOPE)
    else()
      set(profile 330 PARENT_SCOPE)
    endif()
  elseif("${backend}" STREQUAL "metal")
    set(platform osx PARENT_SCOPE)
    set(profile metal PARENT_SCOPE)
  elseif("${backend}" STREQUAL "dx11")
    set(platform windows PARENT_SCOPE)
    if("${stage}" STREQUAL "vs")
      set(profile vs_5_0 PARENT_SCOPE)
    elseif("${stage}" STREQUAL "fs")
      set(profile ps_5_0 PARENT_SCOPE)
    else()
      set(profile cs_5_0 PARENT_SCOPE)
    endif()
  else()
    message(FATAL_ERROR "Unsupported shader backend '${backend}'")
  endif()
endfunction()

function(render_compile_stage program category stage backend source_rel variant defines)
  set(source_path "${SHADER_SOURCE_ROOT}/${source_rel}")
  if(NOT EXISTS "${source_path}")
    message(FATAL_ERROR "Shader source not found for ${program}/${stage}: ${source_path}")
  endif()

  render_backend_profile("${backend}" "${stage}" platform profile)

  if("${stage}" STREQUAL "vs")
    set(shader_type "vertex")
  elseif("${stage}" STREQUAL "fs")
    set(shader_type "fragment")
  elseif("${stage}" STREQUAL "cs")
    set(shader_type "compute")
  else()
    message(FATAL_ERROR "Unsupported shader stage '${stage}'")
  endif()

  set(output_dir "${SHADER_OUTPUT_BIN_ROOT}/${backend}/${category}/${program}/${variant}")
  file(MAKE_DIRECTORY "${output_dir}")
  set(output_file "${output_dir}/${stage}.bin")

  set(stage_args
    --type ${shader_type}
    --platform ${platform}
    --profile ${profile}
    --varyingdef "${SHADER_ROOT}/includes/varying_color.def.sc"
    --include-dir "${SHADER_ROOT}"
    --include-dir "${SHADER_ROOT}/includes"
    -f "${source_path}"
    -o "${output_file}"
  )

  if(defines)
    list(APPEND stage_args --define "${defines}")
  endif()

  execute_process(
    COMMAND "${RENDER_BGFX_SHADERC}" ${stage_args}
    RESULT_VARIABLE shader_result
    OUTPUT_VARIABLE shader_stdout
    ERROR_VARIABLE shader_stderr
  )

  if(NOT shader_result EQUAL 0)
    message(FATAL_ERROR
      "Failed to compile shader\n"
      "  program=${program}\n"
      "  category=${category}\n"
      "  stage=${stage}\n"
      "  backend=${backend}\n"
      "  variant=${variant}\n"
      "  source=${source_rel}\n"
      "  defines=${defines}\n"
      "  stdout=${shader_stdout}\n"
      "  stderr=${shader_stderr}")
  endif()

  file(SHA256 "${output_file}" artifact_hash)

  set(meta_dir "${SHADER_OUTPUT_META_ROOT}/${backend}/${category}/${program}/${variant}")
  file(MAKE_DIRECTORY "${meta_dir}")
  set(meta_file "${meta_dir}/${stage}.json")

  string(REPLACE "\"" "\\\"" safe_defines "${defines}")

  file(WRITE "${meta_file}" "{\n")
  file(APPEND "${meta_file}" "  \"schema_version\": 1,\n")
  file(APPEND "${meta_file}" "  \"program\": \"${program}\",\n")
  file(APPEND "${meta_file}" "  \"category\": \"${category}\",\n")
  file(APPEND "${meta_file}" "  \"variant\": \"${variant}\",\n")
  file(APPEND "${meta_file}" "  \"stage\": \"${stage}\",\n")
  file(APPEND "${meta_file}" "  \"backend\": \"${backend}\",\n")
  file(APPEND "${meta_file}" "  \"source\": \"${source_rel}\",\n")
  file(APPEND "${meta_file}" "  \"compiled_binary\": \"${backend}/${category}/${program}/${variant}/${stage}.bin\",\n")
  file(APPEND "${meta_file}" "  \"defines\": \"${safe_defines}\",\n")
  file(APPEND "${meta_file}" "  \"binary_sha256\": \"${artifact_hash}\"\n")
  file(APPEND "${meta_file}" "}\n")

  render_shader_log("[ok] ${backend}/${category}/${program}/${variant}/${stage}.bin")
endfunction()

function(render_emit_program_metadata program category backend variant defines)
  set(meta_dir "${SHADER_OUTPUT_META_ROOT}/${backend}/${category}/${program}/${variant}")
  file(MAKE_DIRECTORY "${meta_dir}")
  set(program_file "${meta_dir}/program.json")
  string(REPLACE "\"" "\\\"" safe_defines "${defines}")

  file(WRITE "${program_file}" "{\n")
  file(APPEND "${program_file}" "  \"schema_version\": 1,\n")
  file(APPEND "${program_file}" "  \"program\": \"${program}\",\n")
  file(APPEND "${program_file}" "  \"category\": \"${category}\",\n")
  file(APPEND "${program_file}" "  \"backend\": \"${backend}\",\n")
  file(APPEND "${program_file}" "  \"variant\": \"${variant}\",\n")
  file(APPEND "${program_file}" "  \"stages\": [\"vs\", \"fs\"],\n")
  file(APPEND "${program_file}" "  \"defines\": \"${safe_defines}\",\n")
  file(APPEND "${program_file}" "  \"vertex_binary\": \"${backend}/${category}/${program}/${variant}/vs.bin\",\n")
  file(APPEND "${program_file}" "  \"fragment_binary\": \"${backend}/${category}/${program}/${variant}/fs.bin\",\n")
  file(APPEND "${program_file}" "  \"reflection_files\": [\n")
  file(APPEND "${program_file}" "    \"${backend}/${category}/${program}/${variant}/vs.json\",\n")
  file(APPEND "${program_file}" "    \"${backend}/${category}/${program}/${variant}/fs.json\"\n")
  file(APPEND "${program_file}" "  ]\n")
  file(APPEND "${program_file}" "}\n")

  render_shader_log("[meta] ${backend}/${category}/${program}/${variant}/program.json")
endfunction()

file(MAKE_DIRECTORY "${SHADER_OUTPUT_BIN_ROOT}")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_META_ROOT}")
render_shader_log("shaderc: ${RENDER_BGFX_SHADERC}")
render_shader_log("output root: ${RENDER_SHADER_OUTPUT_ROOT}")

foreach(backend IN LISTS RENDER_SHADER_BACKENDS)
  foreach(entry IN LISTS RENDER_SHADER_PROGRAMS)
    string(REPLACE "|" ";" entry_fields "${entry}")
    list(LENGTH entry_fields entry_count)
    if(NOT entry_count EQUAL 6)
      message(FATAL_ERROR "Invalid shader manifest entry '${entry}'. Expected 6 fields")
    endif()

    list(GET entry_fields 0 program)
    list(GET entry_fields 1 category)
    list(GET entry_fields 2 vs_source)
    list(GET entry_fields 3 fs_source)
    list(GET entry_fields 4 variant)
    list(GET entry_fields 5 defines)

    render_compile_stage("${program}" "${category}" vs "${backend}" "${vs_source}" "${variant}" "${defines}")
    render_compile_stage("${program}" "${category}" fs "${backend}" "${fs_source}" "${variant}" "${defines}")
    render_emit_program_metadata("${program}" "${category}" "${backend}" "${variant}" "${defines}")
  endforeach()
endforeach()

message(STATUS "Shader pipeline compilation complete: ${RENDER_SHADER_OUTPUT_ROOT}")
