// shadertype=glsl
#version 450
out vec4 FragColor;
in vec2 thefrag_TexCoord;

uniform sampler2D uni_albedoTexture;
uniform bool uni_useAlbedoTexture;
uniform vec4 uni_albedo;

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