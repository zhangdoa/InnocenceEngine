// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0, binding = 0) uniform sampler2D uni_normalTexture;

layout(location = 0) out VS_OUT {
	vec3 normal;
} vs_out;

layout(location = 1) uniform mat4 uni_p;
layout(location = 2) uniform mat4 uni_r;
layout(location = 3) uniform mat4 uni_t;
layout(location = 4) uniform mat4 uni_m_local;

void main()
{
	mat3 normalMatrix = mat3(transpose(inverse(uni_r * uni_t * uni_m_local)));
	vs_out.normal = normalize(vec3(uni_p * vec4(normalMatrix * inNormal.xyz, 0.0) * texture(uni_normalTexture, inTexCoord)));
	gl_Position = uni_p * uni_r * uni_t * uni_m_local * inPosition;
}