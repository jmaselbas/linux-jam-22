#version 300 es
precision highp float;
precision highp int;
precision mediump sampler2D;
precision highp sampler2DShadow;

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
uniform sampler2D t_line;
uniform sampler2D depth;
#define shadowmap depth

const float PI = 3.141592;
const float Epsilon = 0.00001;

const float fogdensity = 0.005;
const vec3 fogcolor = vec3(0.5, 0.4, 0.3);
const vec3 bot = vec3(0.40, 0.5, 0.47);
const vec3 top = vec3(0.10, 0.14, 0.2);

#define HEX2RGB(h) vec3(float((h >> 16) & 0xff) / 255.0, float((h >>  8) & 0xff) / 255.0, float((h >>  0) & 0xff) / 255.0)

float shadowlookup(vec4 coord, vec2 uv)
{
	vec2 off = uv / vec2(textureSize(shadowmap, 0));
	vec3 p = coord.xyz;

	if (p.z > (texture(shadowmap, p.xy + off).r + 0.00001))
		return 1.0;
	else
		return 0.0;
}

float pcf_shadow(void)
{
	float shadow = 0.0;
	int x, y;
	/* PCF: Percentage close filtering */
	for (x = -1; x <= 1; ++x)
		for (y = -1; y <= 1; ++y)
			shadow += shadowlookup(shadowpos, vec2(x, y));
	return shadow / 9.0;
}

vec3 vline(vec2 p)
{
	vec2 pos = p;
	vec2 axi = normalize(vec2(1.0, -1.4));
	float f = dot(axi, pos);
	vec2 uv = (pos + f*axi);
	vec3 t = texture(t_line, uv).rgb;
	return t;
}

void main(void)
{
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float shadow = 0.0;
	float inc = -dot(normal, lightd);

	if (inc < 0.0)
		shadow = 1.0;
	else
		shadow = pcf_shadow();

	vec3 col, lin;
	float val;

	lin = vline(position.xz *0.5+ position.yz*0.5);
	if (shadow > 0.15)
		val = smoothstep(0.1, 0.8, (lin.r * lin.g));
	else
		val = smoothstep(0.0, 0.2, (lin.r + 2.0 * inc * lin.g));
	col = vec3(1.0) * clamp(val, 0.0, 1.0);

	col += mix(color, vec3(0), clamp(position.y * 4.0, 0.0, 1.0));
	vec3 v = normalize(camp - position);
	float yy = 1.0 - dot(normalize(vec3(0.0, 1.0, 0.0)), v);
	float den = fogdensity * mix(0.01, 1.0, clamp(yy, 0.0, 1.0));
	float fog = exp(-den* z * z);
	col = mix(fogcolor, col, clamp(fog, 0.0, 1.0));

	out_color = vec4(col, 1.0);
}
