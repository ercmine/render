function(render_setup_versioning)
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git rev-parse --short=12 HEAD
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE git_result
      OUTPUT_VARIABLE git_hash
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )

    if(git_result EQUAL 0)
      set(RENDER_GIT_HASH "${git_hash}" CACHE STRING "Git commit hash" FORCE)
    else()
      set(RENDER_GIT_HASH "unknown" CACHE STRING "Git commit hash" FORCE)
    endif()
  else()
    set(RENDER_GIT_HASH "unknown" CACHE STRING "Git commit hash" FORCE)
  endif()

  message(STATUS "Version metadata: ${PROJECT_VERSION} (${RENDER_GIT_HASH})")
endfunction()
