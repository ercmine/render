#ifndef RENDER_DEBUG_COLOR_SH
#define RENDER_DEBUG_COLOR_SH

vec4 render_debug_tint(vec4 color)
{
#if defined(RENDER_DEBUG_TINT)
    return color * vec4(1.0, 0.9, 0.9, 1.0);
#else
    return color;
#endif
}

#endif // RENDER_DEBUG_COLOR_SH
