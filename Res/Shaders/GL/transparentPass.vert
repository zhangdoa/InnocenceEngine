// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec4 thefrag_WorldSpacePos;
layout(location = 1) out vec2 thefrag_TexCoord;
layout(location = 2) out vec3 thefrag_Normal;

void main()
{
	// output the fragment position in world space
	thefrag_WorldSpacePos = meshUBO.m * inPosition;

	// output the current and previous fragment position in clip space
	vec4 posVS = cameraUBO.r * cameraUBO.t * thefrag_WorldSpacePos;

	// output the texture coordinate
	thefrag_TexCoord = inTexCoord;

	// output the normal
	thefrag_Normal = mat3(transpose(inverse(meshUBO.m))) * inNormal.xyz;

	gl_Position = cameraUBO.p_jittered * posVS;
}