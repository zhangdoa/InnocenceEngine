#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout(std140, row_major, binding = 0) uniform cameraUBO
{
	mat4 uni_p_camera_original;
	mat4 uni_p_camera_jittered;
	mat4 uni_r_camera;
	mat4 uni_t_camera;
	mat4 uni_r_camera_prev;
	mat4 uni_t_camera_prev;
	vec4 uni_globalPos;
	float WHRatio;
};

void main() {
	// output the fragment position in world space
	thefrag_WorldSpacePos = inPosition;
	vec4 thefrag_WorldSpacePos_prev = inPosition;

	// output the current and previous fragment position in clip space
	vec4 thefrag_CameraSpacePos_current = uni_r_camera * uni_t_camera * thefrag_WorldSpacePos;
	vec4 thefrag_CameraSpacePos_previous = uni_r_camera_prev * uni_t_camera_prev * thefrag_WorldSpacePos_prev;

	thefrag_ClipSpacePos_current = uni_p_camera_original * thefrag_CameraSpacePos_current;
	thefrag_ClipSpacePos_previous = uni_p_camera_original * thefrag_CameraSpacePos_previous;

	// output the texture coordinate
	thefrag_TexCoord = inTexCoord;

	// output the normal
	thefrag_Normal = inNormal.xyz;

	gl_Position = uni_p_camera_jittered * thefrag_CameraSpacePos_current;
}