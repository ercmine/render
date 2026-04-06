$input v_color0

#include "../includes/core/common.sh"
#include "../includes/debug/debug_color.sh"

void main()
{
    gl_FragColor = render_debug_tint(v_color0);
}
