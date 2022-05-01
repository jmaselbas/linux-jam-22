#version 330 core
precision highp float;
precision highp int;

in vec3 normal;
in vec3 position;
in vec2 texcoord;
in vec4 shadowpos;

out vec4 out_color;

uniform vec3 camp;
uniform vec3 lightd;
uniform vec4 lightc;
uniform float time;

uniform vec3 color;

const float fogdensity = 0.005;
const vec3 fogcolor = vec3(0.5, 0.4, 0.3);
const vec3 bot = vec3(0.40, 0.5, 0.47);
const vec3 top = vec3(0.10, 0.14, 0.2);

void main(void)
{
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float shadow = 0.0;
	float inc = -dot(normal, lightd);
	vec3 col, lin;
	float val;

	float a = clamp(1.0-position.y*1.4, 0.0, 1.0);
	col = color * a;
	//col = mix(fogcolor, col, clamp(fog, 0.0, 1.0));
	out_color = vec4(col, a);
}
