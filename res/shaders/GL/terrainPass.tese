// shadertype=glsl
#version 450

layout(quads, equal_spacing, ccw) in;

layout(location = 0, binding = 0) uniform sampler2D uni_heightTexture;
layout(location = 1, binding = 1) uniform sampler2D uni_normalTexture;

layout(location = 0) out TES_OUT
{
	vec4 normal;
}tes_out;

void main()
{      
	float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
	vec4 a = mix(gl_in[1].gl_Position, gl_in[0].gl_Position, u);
    vec4 b = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, u);
    vec4 position = mix(a, b, v);

	vec2 heightTextureSize = vec2(textureSize(uni_heightTexture, 0));
	int textureScale = int(heightTextureSize.x);

	vec2 texCoord = vec2(position.xz * 4 / textureScale);
	float height = texture(uni_heightTexture, texCoord).r;
	height = height * 2.0f - 1.0f;

	tes_out.normal = texture(uni_normalTexture, texCoord);

	// tangent to world space
	tes_out.normal.xyz = tes_out.normal.xzy;

	vec4 localSpacePos = vec4(position.x, height, position.z, 1.0f);
	localSpacePos.y = height;

	gl_Position = localSpacePos;
}