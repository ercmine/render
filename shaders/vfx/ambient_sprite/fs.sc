$input v_color0

#include "../../includes/core/common.sh"

void main()
{
    vec3 tint = vec3(0.75, 0.85, 1.0);
#if defined(VFX_DUST)
    tint = vec3(0.55, 0.55, 0.58);
#elif defined(VFX_EMBERS)
    tint = vec3(1.0, 0.52, 0.20);
#elif defined(VFX_RESIN_DRIP)
    tint = vec3(0.95, 0.67, 0.28);
#elif defined(VFX_GLOW_PULSE)
    tint = vec3(0.35, 0.95, 1.0);
#endif

    gl_FragColor = vec4(v_color0.rgb * tint, 0.45);
}
