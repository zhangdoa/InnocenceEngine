// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec4 thefrag_WorldSpacePos;
layout(location = 1) out vec4 thefrag_ClipSpacePos_current;
layout(location = 2) out vec4 thefrag_ClipSpacePos_previous;
layout(location = 3) out vec2 thefrag_TexCoord;
layout(location = 4) out vec3 thefrag_Normal;
layout(location = 5) out float thefrag_UUID;

void main()
{
	// output the fragment position in world space
	thefrag_WorldSpacePos = meshUBO.m * inPosition;
	vec4 thefrag_WorldSpacePos_prev = meshUBO.m_prev * inPosition;

	// output the current and previous fragment position in clip space
	vec4 thefrag_CameraSpacePos_current = cameraUBO.r * cameraUBO.t * thefrag_WorldSpacePos;
	vec4 thefrag_CameraSpacePos_previous = cameraUBO.r_prev * cameraUBO.t_prev * thefrag_WorldSpacePos_prev;

	thefrag_ClipSpacePos_current = cameraUBO.p_original * thefrag_CameraSpacePos_current;
	thefrag_ClipSpacePos_previous = cameraUBO.p_original * thefrag_CameraSpacePos_previous;

	// output the texture coordinate
	thefrag_TexCoord = inTexCoord;

	// output the normal
	thefrag_Normal = mat3(transpose(inverse(meshUBO.m))) * inNormal.xyz;

	// output the UUID
	thefrag_UUID = meshUBO.UUID;

	gl_Position = cameraUBO.p_jittered * thefrag_CameraSpacePos_current;
}