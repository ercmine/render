if(NOT DEFINED RENDER_PACKAGE_STAGE_DIR)
  message(FATAL_ERROR "RENDER_PACKAGE_STAGE_DIR is required")
endif()

if(NOT DEFINED RENDER_PACKAGE_BIN_SUBDIR)
  set(RENDER_PACKAGE_BIN_SUBDIR "bin")
endif()

if(NOT DEFINED RENDER_PACKAGE_SHADER_RELATIVE_DIR)
  set(RENDER_PACKAGE_SHADER_RELATIVE_DIR "shaders/bin")
endif()

if(NOT DEFINED RENDER_PACKAGE_LOG_FILE)
  set(RENDER_PACKAGE_LOG_FILE "")
endif()

function(render_package_log line)
  if(RENDER_PACKAGE_LOG_FILE)
    file(APPEND "${RENDER_PACKAGE_LOG_FILE}" "${line}\n")
  endif()
endfunction()

if(RENDER_PACKAGE_LOG_FILE)
  file(WRITE "${RENDER_PACKAGE_LOG_FILE}" "render package validation log\n")
endif()

set(stage_dir "${RENDER_PACKAGE_STAGE_DIR}")
set(bin_dir "${stage_dir}/${RENDER_PACKAGE_BIN_SUBDIR}")
set(shader_dir "${stage_dir}/${RENDER_PACKAGE_SHADER_RELATIVE_DIR}")
set(shader_metadata_dir "${stage_dir}/shaders/metadata")

if(WIN32)
  set(shell_name "render_shell.exe")
else()
  set(shell_name "render_shell")
endif()

set(shell_path "${bin_dir}/${shell_name}")
if(NOT EXISTS "${shell_path}")
  message(FATAL_ERROR "Packaging validation failed: missing executable ${shell_path}")
endif()
render_package_log("[ok] executable: ${shell_path}")

if(NOT EXISTS "${shader_dir}")
  message(FATAL_ERROR "Packaging validation failed: missing shader directory ${shader_dir}")
endif()
render_package_log("[ok] shader directory: ${shader_dir}")

file(GLOB_RECURSE compiled_shaders
  "${shader_dir}/*.bin"
)
list(LENGTH compiled_shaders shader_count)
if(shader_count EQUAL 0)
  message(FATAL_ERROR "Packaging validation failed: no compiled shader binaries found in ${shader_dir}")
endif()
render_package_log("[ok] compiled shader count: ${shader_count}")

if(NOT EXISTS "${shader_metadata_dir}")
  message(FATAL_ERROR "Packaging validation failed: missing shader metadata directory ${shader_metadata_dir}")
endif()
render_package_log("[ok] shader metadata directory: ${shader_metadata_dir}")

file(GLOB_RECURSE shader_metadata_files
  "${shader_metadata_dir}/*.json"
)
list(LENGTH shader_metadata_files metadata_count)
if(metadata_count EQUAL 0)
  message(FATAL_ERROR "Packaging validation failed: no shader metadata files found in ${shader_metadata_dir}")
endif()
render_package_log("[ok] shader metadata count: ${metadata_count}")

set(archive_base "${stage_dir}/../render-package")
if(WIN32)
  set(archive_path "${archive_base}.zip")
else()
  set(archive_path "${archive_base}.tar.gz")
endif()

file(MAKE_DIRECTORY "${stage_dir}/../")

if(WIN32)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${archive_path}" --format=zip .
    WORKING_DIRECTORY "${stage_dir}"
    RESULT_VARIABLE archive_result
  )
else()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${archive_path}" .
    WORKING_DIRECTORY "${stage_dir}"
    RESULT_VARIABLE archive_result
  )
endif()

if(NOT archive_result EQUAL 0)
  message(FATAL_ERROR "Packaging validation failed: archive creation failed")
endif()

render_package_log("[ok] archive: ${archive_path}")
message(STATUS "Packaging validation complete: ${archive_path}")
