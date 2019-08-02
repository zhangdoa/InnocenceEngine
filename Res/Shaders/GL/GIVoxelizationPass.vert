// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out VS_OUT {
	vec2 texCoord;
	vec3 normal;
} vs_out;

void main()
{
	vec4 posWS = uni_m * inPosition;

	vs_out.normal = mat3(transpose(inverse(uni_m))) * inNormal.xyz;
	vs_out.texCoord = inTexCoord;

	gl_Position = posWS;
}