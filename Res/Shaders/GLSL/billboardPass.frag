// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_billboardPassRT0;
layout(location = 0) in vec2 thefrag_TexCoord;

layout(set = 1, binding = 0) uniform texture2D uni_texture;

layout(set = 2, binding = 0) uniform sampler samplerLinear;

void main()
{
	vec4 textureColor = texture(sampler2D(uni_texture, samplerLinear), thefrag_TexCoord);
	if (textureColor.a == 0.0)
		discard;
	uni_billboardPassRT0 = vec4(textureColor.rgb, 1.0);
}