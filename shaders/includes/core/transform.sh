#ifndef RENDER_TRANSFORM_SH
#define RENDER_TRANSFORM_SH

uniform mat4 u_modelViewProj;

vec4 render_transform_position(vec3 position)
{
    return mul(u_modelViewProj, vec4(position, 1.0));
}

#endif // RENDER_TRANSFORM_SH
