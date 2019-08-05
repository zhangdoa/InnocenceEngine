// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out VS_OUT
{
	vec4 posWS;
	vec2 texcoord;
	vec4 normal;
} vs_out;

void main()
{
	// output the fragment position in world space
	vs_out.posWS = meshUBO.m * inPosition;

	// output the texture coordinate
	vs_out.texcoord = inTexCoord;

	// output the normal
	vs_out.normal = vec4(mat3(transpose(inverse(meshUBO.m))) * inNormal.xyz, 0.0f);

	gl_Position = vs_out.posWS;
}