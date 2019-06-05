// shadertype=<glsl>
#version 450
layout(location = 0) out vec4 uni_billboardPassRT0;
layout(location = 0) in vec2 thefrag_TexCoord;

layout(location = 5, binding = 0) uniform sampler2D uni_texture;

void main()
{
	vec4 textureColor = texture(uni_texture, thefrag_TexCoord);
	if (textureColor.a == 0.0)
		discard;
	uni_billboardPassRT0 = vec4(textureColor.rgb, 1.0);
}