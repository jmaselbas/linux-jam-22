#version 330 core
precision highp float;
precision highp int;

in vec3 normal;
in vec2 texcoord;
out vec4 out_color;

uniform float time;
uniform vec3 color;
uniform sampler2D tex;

void main(void)
{
	vec2 uv = texcoord;
	vec3 col = texture(tex, uv).rgb;
	out_color = vec4(col, 1.0);
}
