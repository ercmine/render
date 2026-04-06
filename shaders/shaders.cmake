# Canonical shader manifest for render.
# Each entry: program|category|vertex_source|fragment_source|variant|defines
set(RENDER_SHADER_PROGRAMS
  "debug_triangle|debug|debug/triangle/vs.sc|debug/triangle/fs.sc|default|"
  "debug_triangle|debug|debug/triangle/vs.sc|debug/triangle/fs.sc|debug_tint|RENDER_DEBUG_TINT=1"
  "renderer_debug_normals|debug|debug/renderer_debug/vs.sc|debug/renderer_debug/fs_normals.sc|default|"
  "renderer_debug_albedo|debug|debug/renderer_debug/vs.sc|debug/renderer_debug/fs_albedo.sc|default|"
  "renderer_debug_depth|debug|debug/renderer_debug/vs.sc|debug/renderer_debug/fs_depth.sc|default|"
  "renderer_debug_overdraw|debug|debug/renderer_debug/vs.sc|debug/renderer_debug/fs_overdraw.sc|default|"
  "material_unlit_color|materials|materials/unlit_color/vs.sc|materials/unlit_color/fs.sc|default|"
  "ambient_sprite|vfx|vfx/ambient_sprite/vs.sc|vfx/ambient_sprite/fs.sc|spores|VFX_SPORES=1"
  "ambient_sprite|vfx|vfx/ambient_sprite/vs.sc|vfx/ambient_sprite/fs.sc|dust|VFX_DUST=1"
  "ambient_sprite|vfx|vfx/ambient_sprite/vs.sc|vfx/ambient_sprite/fs.sc|embers|VFX_EMBERS=1"
  "ambient_sprite|vfx|vfx/ambient_sprite/vs.sc|vfx/ambient_sprite/fs.sc|resin_drip|VFX_RESIN_DRIP=1"
  "ambient_sprite|vfx|vfx/ambient_sprite/vs.sc|vfx/ambient_sprite/fs.sc|glow_pulse|VFX_GLOW_PULSE=1"
)

# Optional future compute entries: name|category|compute_source|variant|defines
set(RENDER_SHADER_COMPUTE_PROGRAMS)
