// shadertype=glsl
#include "common.glsl"

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

layout(location = 0) out vec4 thefrag_WorldSpacePos;
layout(location = 1) out vec4 thefrag_ClipSpacePos_current;
layout(location = 2) out vec4 thefrag_ClipSpacePos_previous;
layout(location = 3) out vec2 thefrag_TexCoord;
layout(location = 4) out vec3 thefrag_Normal;
layout(location = 5) out float thefrag_UUID;

void main()
{
	// output the fragment position in world space
	thefrag_WorldSpacePos = uni_m * vec4(in_Position, 1.0);
	vec4 thefrag_WorldSpacePos_prev = uni_m_prev * vec4(in_Position, 1.0);

	// output the current and previous fragment position in clip space
	vec4 thefrag_CameraSpacePos_current = uni_r_camera * uni_t_camera * thefrag_WorldSpacePos;
	vec4 thefrag_CameraSpacePos_previous = uni_r_camera_prev * uni_t_camera_prev * thefrag_WorldSpacePos_prev;

	thefrag_ClipSpacePos_current = uni_p_camera_original * thefrag_CameraSpacePos_current;
	thefrag_ClipSpacePos_previous = uni_p_camera_original * thefrag_CameraSpacePos_previous;

	// output the texture coordinate
	thefrag_TexCoord = in_TexCoord;

	// output the normal
	thefrag_Normal = mat3(transpose(inverse(uni_m))) * in_Normal;

	// output the UUID
	thefrag_UUID = uni_UUID;

	gl_Position = uni_p_camera_jittered * thefrag_CameraSpacePos_current;
}