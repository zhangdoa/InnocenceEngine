// shadertype=<glsl>
#version 450
layout(location = 0) out vec4 uni_preTAAPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_lightPassRT0;
layout(location = 1, binding = 1) uniform sampler2D uni_skyPassRT0;
layout(location = 2, binding = 2) uniform sampler2D uni_terrainPassRT0;

void main()
{
	vec4 lightPassResult = texture(uni_lightPassRT0, TexCoords);
	vec4 skyPassResult = texture(uni_skyPassRT0, TexCoords);
	vec4 terrainPassResult = texture(uni_terrainPassRT0, TexCoords);

	vec3 finalColor = vec3(0.0);

	finalColor += terrainPassResult.rgb;

	if (lightPassResult.a == 0.0)
	{
		finalColor += skyPassResult.rgb;
	}
	else
	{
		finalColor += lightPassResult.rgb;
	}

	uni_preTAAPassRT0 = vec4(finalColor, 1.0);
}