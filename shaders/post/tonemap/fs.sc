$input v_texcoord0

SAMPLER2D(s_color, 0);

void main()
{
    vec4 color = texture2D(s_color, v_texcoord0);
    gl_FragColor = color / (color + vec4(1.0));
}
