// shadertype=glsl
#version 450
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

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

out GS_OUT
{
	vec4 normal;
} gs_out;

void localSpaceToClipSpace(unsigned int index)
{
	vec4 thefrag_CameraSpacePos = uni_r_camera * uni_t_camera * gl_in[index].gl_Position;
	gl_Position = uni_p_camera_original * thefrag_CameraSpacePos;
	EmitVertex();
}

void main()
{
	vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 faceNormal = cross(p1, p2);

	gs_out.normal = vec4(faceNormal, 1.0f);

	localSpaceToClipSpace(0);
	localSpaceToClipSpace(1);
	localSpaceToClipSpace(2);

	EndPrimitive();
}