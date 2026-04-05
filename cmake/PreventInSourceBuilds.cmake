function(render_prevent_in_source_builds)
  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
      "In-source builds are not allowed.\n"
      "Configure with a preset, for example:\n"
      "  cmake --preset linux-debug\n"
    )
  endif()
endfunction()
