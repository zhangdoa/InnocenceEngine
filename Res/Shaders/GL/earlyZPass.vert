// shadertype=glsl
#include "common.glsl"

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

void main()
{
	vec4 thefrag_WorldSpacePos = uni_m * vec4(in_Position, 1.0);

	vec4 thefrag_CameraSpacePos_current = uni_r_camera * uni_t_camera * thefrag_WorldSpacePos;

	gl_Position = uni_p_camera_jittered * thefrag_CameraSpacePos_current;
}