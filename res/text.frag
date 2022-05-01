#version 300 es
precision highp float;
precision highp int;

in vec3 normal;
in vec2 texcoord;
out vec4 out_color;

uniform vec3 color;
uniform sampler2D tex;

void main(void)
{
	vec2 uv = texcoord;
	float alpha = texture(tex, uv).a;
	vec3 col = color;
	alpha = smoothstep(0.49, 0.54, alpha);
	out_color = vec4(col, alpha);
}
