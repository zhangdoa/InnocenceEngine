// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_normal;
layout(location = 0) in vec2 texCoords;
layout(location = 0, binding = 0) uniform sampler2D uni_heightTexture;

void main()
{
	vec2 textureSize = vec2(textureSize(uni_heightTexture, 0));
	vec2 texelSize = 1.0 / textureSize;

	float heightT = texture(uni_heightTexture, texCoords - vec2(0.0, texelSize.y)).r;
	float heightB = texture(uni_heightTexture, texCoords + vec2(0.0, texelSize.y)).r;
	float heightL = texture(uni_heightTexture, texCoords - vec2(texelSize.x, 0.0)).r;
	float heightR = texture(uni_heightTexture, texCoords + vec2(texelSize.x, 0.0)).r;

	float dp1 = heightR - heightL;
	float dp2 = heightT - heightB;

	vec3 normal = normalize(vec3(dp1, dp2, 2.0));
	uni_normal = vec4(normal * 0.5 + 0.5, 1.0);
}