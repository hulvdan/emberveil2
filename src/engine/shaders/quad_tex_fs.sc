$input v_texcoord0, v_color0, v_color1
$uniform u_texture0 0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

void main() {
    vec4 tex = texture2D(s_texColor, v_texcoord0);
    vec4 c = tex * v_color0;
    c.rgb = mix(c.rgb, v_color1.rgb, v_color1.a);
    gl_FragColor = c;
}
