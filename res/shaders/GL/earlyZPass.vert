// shadertype=glsl
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

layout(row_major) uniform cameraUBO
{
	mat4 uni_p_camera_original;
	mat4 uni_p_camera_jittered;
	mat4 uni_r_camera;
	mat4 uni_t_camera;
	mat4 uni_r_camera_prev;
	mat4 uni_t_camera_prev;
};

layout(row_major) uniform meshUBO
{
	mat4 uni_m;
	mat4 uni_m_prev;
};

void main()
{
	vec4 thefrag_WorldSpacePos = uni_m * vec4(in_Position, 1.0);

	vec4 thefrag_CameraSpacePos_current = uni_r_camera * uni_t_camera * thefrag_WorldSpacePos;

	gl_Position = uni_p_camera_jittered * thefrag_CameraSpacePos_current;
}