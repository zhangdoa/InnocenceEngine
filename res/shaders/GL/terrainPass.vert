// shadertype=glsl
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

layout(location = 0) out vec4 thefrag_WorldSpacePos;
layout(location = 1) out vec2 thefrag_TexCoord;
layout(location = 2) out vec3 thefrag_Normal;

layout(location = 0) uniform mat4 uni_p_camera;
layout(location = 1) uniform mat4 uni_r_camera;
layout(location = 2) uniform mat4 uni_t_camera;

layout(location = 3) uniform mat4 uni_m;

layout(location = 4, binding = 0) uniform sampler2D uni_heightTexture;

void main()
{
	vec2 renderTargetSize = vec2(textureSize(uni_heightTexture, 0));
	int textureSize = int(renderTargetSize.x);
	vec3 height = texture(uni_heightTexture, in_TexCoord).rgb;
	height = height * 2.0f - 1.0f;

	vec4 localSpacePos = vec4(in_Position, 1.0);
	localSpacePos.y = height.y * textureSize / 8.0f;

	int l_localIndex = gl_VertexID % 4;

	if (l_localIndex == 0)
	{
		thefrag_TexCoord = vec2(1.0f, 1.0f);
	}
	else if (l_localIndex == 1)
	{
		thefrag_TexCoord = vec2(0.0f, 1.0f);
	}
	else if (l_localIndex == 2)
	{
		thefrag_TexCoord = vec2(1.0f, 0.0f);
	}
	else if (l_localIndex == 3)
	{
		thefrag_TexCoord = vec2(0.0f, 0.0f);
	}

	thefrag_WorldSpacePos = uni_m * localSpacePos;

	vec4 thefrag_CameraSpacePos = uni_r_camera * uni_t_camera * thefrag_WorldSpacePos;

	thefrag_Normal = mat3(transpose(inverse(uni_m))) * in_Normal;

	gl_Position = uni_p_camera * thefrag_CameraSpacePos;
}