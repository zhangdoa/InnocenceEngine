// shadertype=glsl
#include "common.glsl"
layout(location = 0) out vec4 uni_outputTexture;
layout(location = 0) in vec2 thefrag_TexCoord;

layout(location = 0) uniform bool uni_horizontal;
layout(location = 1) uniform uint uni_kernel = 0;
layout(location = 2, binding = 0) uniform sampler2D uni_inputTexture;

// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
const float weight9[3] = float[](0.294118, 0.235294, 0.117647);
const float weight13[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
const float weight17[7] = float[](0.196483, 0.174651, 0.122256, 0.066685, 0.027786, 0.008549, 0.001832);

vec4 GaussianBlur9(sampler2D image)
{
	vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
	vec3 result = texture(image, thefrag_TexCoord).rgb * weight9[0]; // current fragment's contribution
	if (uni_horizontal)
	{
		for (int i = 1; i < 3; ++i)
		{
			result += texture(image, thefrag_TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight9[i];
			result += texture(image, thefrag_TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight9[i];
		}
	}
	else
	{
		for (int i = 1; i < 3; ++i)
		{
			result += texture(image, thefrag_TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight9[i];
			result += texture(image, thefrag_TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight9[i];
		}
	}
	return vec4(result, 0.0);
}

vec4 GaussianBlur13(sampler2D image)
{
	vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
	vec3 result = texture(image, thefrag_TexCoord).rgb * weight13[0]; // current fragment's contribution
	if (uni_horizontal)
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(image, thefrag_TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight13[i];
			result += texture(image, thefrag_TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight13[i];
		}
	}
	else
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(image, thefrag_TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight13[i];
			result += texture(image, thefrag_TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight13[i];
		}
	}
	return vec4(result, 0.0);
}

vec4 GaussianBlur17(sampler2D image)
{
	vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
	vec3 result = texture(image, thefrag_TexCoord).rgb * weight17[0]; // current fragment's contribution
	if (uni_horizontal)
	{
		for (int i = 1; i < 7; ++i)
		{
			result += texture(image, thefrag_TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight17[i];
			result += texture(image, thefrag_TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight17[i];
		}
	}
	else
	{
		for (int i = 1; i < 7; ++i)
		{
			result += texture(image, thefrag_TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight17[i];
			result += texture(image, thefrag_TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight17[i];
		}
	}
	return vec4(result, 0.0);
}

void main()
{
	if (uni_kernel == 0)
	{
		uni_outputTexture = GaussianBlur9(uni_inputTexture);
	}
	else if (uni_kernel == 1)
	{
		uni_outputTexture = GaussianBlur13(uni_inputTexture);
	}
	else
	{
		uni_outputTexture = GaussianBlur17(uni_inputTexture);
	}
}