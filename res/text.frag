#version 330 core
precision highp float;
precision highp int;

in vec3 normal;
in vec2 texcoord;
out vec4 out_color;

uniform float time;
uniform vec3 color;
uniform sampler2D tex;
uniform float fx;

float sqr(float x)
{
if (x < 0.5)
   return 1.;
else
   return 0.;
}

void main(void)
{
    vec2 uv = texcoord;
    if (fx > 0.)
       uv.x += 0.008 * sin( 10. * sqr(sin(uv.y * time)) * uv.y + time * 5.);

    float alpha = texture(tex, uv).r;
    vec3 col = color;
    alpha = smoothstep(0.49, 0.54, alpha);
	out_color = vec4(col, alpha);
}
