// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec3 thefrag_Normal;

layout(location = 0) uniform mat4 uni_m_local;

void main()
{
	// output the fragment position in world space
	vec4 posWS = uni_m_local * inPosition;
	vec4 posVS = uni_r_camera * uni_t_camera * posWS;

	thefrag_Normal = mat3(transpose(inverse(uni_m_local))) * inNormal.xyz;

	gl_Position = uni_p_camera_original * posVS;
}