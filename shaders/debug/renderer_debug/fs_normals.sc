$input v_color0, v_world_pos

#include "../../includes/core/common.sh"

void main()
{
    vec3 n = normalize(v_world_pos * 0.5 + vec3(0.001, 0.001, 1.0));
    gl_FragColor = vec4(n * 0.5 + 0.5, 1.0);
}
