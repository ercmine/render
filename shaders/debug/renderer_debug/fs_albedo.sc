$input v_color0, v_world_pos

#include "../../includes/core/common.sh"

void main()
{
    gl_FragColor = vec4(v_color0.rgb, 1.0);
}
