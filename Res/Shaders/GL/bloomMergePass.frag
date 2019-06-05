// shadertype=<glsl>
#version 450
layout(location = 0) out vec4 uni_bloomMergePassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_Full;

void main()
{
	vec3 finalColor = texture(uni_Full, TexCoords).rgb;
	uni_bloomMergePassRT0 = vec4(finalColor, 1.0f);
}