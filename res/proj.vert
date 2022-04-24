#version 330 core

in vec3 in_pos;
in vec3 in_normal;
in vec2 in_texcoord;
out vec3 normal;
out vec3 position;
out vec2 texcoord;
out vec4 shadowpos;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lsview;
uniform mat4 lsproj;
uniform float time;

void main(void)
{
	vec4 pos = model * vec4(in_pos, 1.0);

	gl_Position = proj * view * pos;
	shadowpos = lsproj * lsview * pos;
	shadowpos.xyz = 0.5 + 0.5 * shadowpos.xyz / shadowpos.w;
	position = pos.xyz;
	texcoord = in_texcoord;
	normal = (model * vec4(in_normal, 0.0)).xyz;
}
