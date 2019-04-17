// shadertype=glsl
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

layout(location = 0) out vec4 thefrag_WorldSpacePos;
layout(location = 1) out vec2 thefrag_TexCoord;
layout(location = 2) out vec3 thefrag_Normal;

layout(row_major) uniform cameraUBO
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

layout(row_major) uniform meshUBO
{
	mat4 uni_m;
	mat4 uni_m_prev;
	mat4 uni_normalMat;
	unsigned int uni_UUID;
};

void main()
{
	// output the fragment position in world space
	thefrag_WorldSpacePos = uni_m * vec4(in_Position, 1.0);

	// output the current and previous fragment position in clip space
	vec4 thefrag_CameraSpacePos_current = uni_r_camera * uni_t_camera * thefrag_WorldSpacePos;

	// output the texture coordinate
	thefrag_TexCoord = in_TexCoord;

	// output the normal
	thefrag_Normal = mat3(transpose(inverse(uni_m))) * in_Normal;

	gl_Position = uni_p_camera_jittered * thefrag_CameraSpacePos_current;
}