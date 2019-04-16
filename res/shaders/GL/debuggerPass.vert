// shadertype=glsl
#version 450
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

layout(location = 0, binding = 0) uniform sampler2D uni_normalTexture;

layout(location = 0) out VS_OUT{
	vec3 normal;
} vs_out;

layout(location = 1) uniform mat4 uni_p;
layout(location = 2) uniform mat4 uni_r;
layout(location = 3) uniform mat4 uni_t;
layout(location = 4) uniform mat4 uni_m;

void main()
{
	mat3 normalMatrix = mat3(transpose(inverse(uni_r * uni_t * uni_m)));
	vs_out.normal = normalize(vec3(uni_p * vec4(normalMatrix * in_Normal, 0.0) * texture(uni_normalTexture, in_TexCoord)));
	gl_Position = uni_p * uni_r * uni_t * uni_m * vec4(in_Position, 1.0);
}