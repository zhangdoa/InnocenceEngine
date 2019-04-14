// shadertype=glsl
#version 450
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec2 thefrag_TexCoord;

layout(location = 3, binding = 0) uniform sampler2D uni_albedoTexture;
layout(location = 4) uniform bool uni_useAlbedoTexture;
layout(location = 5) uniform vec4 uni_albedo;

void main()
{
	vec3 albedo;

	if (uni_useAlbedoTexture)
	{
		vec4 albedoTexture = texture(uni_albedoTexture, thefrag_TexCoord);
		float transparency = albedoTexture.a;
		if (transparency < 0.1)
		{
			discard;
		}
		albedo = albedoTexture.rgb;
	}
	else
	{
		albedo = uni_albedo.rgb;
	}

	FragColor = vec4(albedo, 1.0);
}