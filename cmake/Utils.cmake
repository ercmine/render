function(render_mark_target_placeholder target_name)
  target_compile_definitions(${target_name} INTERFACE RENDER_PLACEHOLDER_TARGET=1)
endfunction()
