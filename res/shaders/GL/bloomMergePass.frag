// shadertype=<glsl>
#version 450
layout(location = 0) out vec4 uni_bloomMergePassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_Full;
layout(location = 1, binding = 1) uniform sampler2D uni_Half;
layout(location = 2, binding = 2) uniform sampler2D uni_Quarter;
layout(location = 3, binding = 3) uniform sampler2D uni_Eighth;

void main()
{
	vec3 finalColor = texture(uni_Full, TexCoords).rgb;
	finalColor += texture(uni_Half, TexCoords).rgb;
	finalColor += texture(uni_Quarter, TexCoords).rgb;
	finalColor += texture(uni_Eighth, TexCoords).rgb;

	uni_bloomMergePassRT0 = vec4(finalColor, 1.0f);
}