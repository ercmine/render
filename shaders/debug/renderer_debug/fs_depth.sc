$input v_color0, v_world_pos

#include "../../includes/core/common.sh"

void main()
{
    float linearDepth = clamp((gl_FragCoord.z - 0.01) / (1.0 - 0.01), 0.0, 1.0);
    gl_FragColor = vec4(vec3(1.0 - linearDepth), 1.0);
}
