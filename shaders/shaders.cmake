# Canonical shader manifest for render.
# Each entry: program|category|vertex_source|fragment_source|variant|defines
set(RENDER_SHADER_PROGRAMS
  "debug_triangle|debug|debug/triangle/vs.sc|debug/triangle/fs.sc|default|"
  "debug_triangle|debug|debug/triangle/vs.sc|debug/triangle/fs.sc|debug_tint|RENDER_DEBUG_TINT=1"
  "material_unlit_color|materials|materials/unlit_color/vs.sc|materials/unlit_color/fs.sc|default|"
)

# Optional future compute entries: name|category|compute_source|variant|defines
set(RENDER_SHADER_COMPUTE_PROGRAMS)
