// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec4 posWS;
layout(location = 1) out vec2 TexCoord;
layout(location = 2) out vec3 Normal;

void main()
{
	// output the fragment position in world space
	posWS = perObjectCBuffer.data.m * inPosition;

	// output the current and previous fragment position in clip space
	vec4 posVS = perFrameCBuffer.data.v * posWS;

	// output the texture coordinate
	TexCoord = inTexCoord;

	// output the normal
	Normal = mat3(transpose(inverse(perObjectCBuffer.data.m))) * inNormal.xyz;

	gl_Position = perFrameCBuffer.data.p_jittered * posVS;
}