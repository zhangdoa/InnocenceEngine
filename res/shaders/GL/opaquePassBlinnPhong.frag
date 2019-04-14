// shadertype=glsl
#version 450
layout(location = 0) out vec4 uni_RT0;
layout(location = 1) out vec4 uni_RT1;
layout(location = 2) out vec4 uni_RT2;
layout(location = 3) out vec4 uni_RT3;

in vec3 thefrag_FragPos;
in vec2 thefrag_TexCoord;
in vec3 thefrag_Normal;

uniform sampler2D uni_normalTexture;
uniform sampler2D uni_diffuseTexture;
uniform sampler2D uni_specularTexture;


void main()
{
	// let glsl calculate partial derivatives!
	vec3 Q1 = dFdx(thefrag_FragPos);
	vec3 Q2 = dFdy(thefrag_FragPos);
	vec2 st1 = dFdx(thefrag_TexCoord);
	vec2 st2 = dFdy(thefrag_TexCoord);

	vec3 N = normalize(thefrag_Normal);
	vec3 T = normalize(Q1*st2.t - Q2 * st1.t);
	vec3 B = -normalize(cross(N, T));

	mat3 TBN = transpose(mat3(T, B, N));

	//alpha should be set to 0 for background rendering pass
	uni_RT0 = vec4(thefrag_FragPos, 0.0);

	uni_RT1 = vec4(normalize((texture(uni_normalTexture, thefrag_TexCoord).rgb * 2.0 - 1.0) * TBN), 1.0);

	// store the diffuse color vector in the third rendering target texture
	uni_RT2 = texture(uni_diffuseTexture, thefrag_TexCoord);

	// store the specular color vector in the fourth rendering target texture
	uni_RT3 = texture(uni_specularTexture, thefrag_TexCoord);
}