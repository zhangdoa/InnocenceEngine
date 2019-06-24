// shadertype=glsl
#version 450
layout(location = 0) in vec3 in_Position;

layout(location = 0) out vec3 TexCoords;

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
	float zNear;
	float zFar;
};

void main()
{
	TexCoords = in_Position * -1.0;
	vec4 pos = uni_p_camera_original * uni_r_camera * -1.0 * vec4(in_Position, 1.0);
	gl_Position = pos.xyww;
}