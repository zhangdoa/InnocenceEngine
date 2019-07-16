// shadertype=glsl
#include "common.glsl"

layout(quads, equal_spacing, cw) in;

layout(location = 0, binding = 0) uniform sampler2D uni_heightTexture;
layout(location = 1, binding = 1) uniform sampler2D uni_normalTexture;

layout(location = 0) out TES_OUT
{
	vec3 positionWS;
	vec4 positionCS;
	vec4 positionCS_prev;
	vec2 texCoord;
	vec3 normal;
}tes_out;

void main()
{      
	float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

	tes_out.texCoord = vec2(u, v);

	vec4 a = mix(gl_in[1].gl_Position, gl_in[0].gl_Position, u);
    vec4 b = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, u);
    vec4 position = mix(a, b, v);

	vec2 heightTextureSize = vec2(textureSize(uni_heightTexture, 0));
	int textureScale = int(heightTextureSize.x);

	vec2 texCoord = vec2(position.xz * 4 / textureScale);
	float height = texture(uni_heightTexture, texCoord).r;
	height = height * 2.0f - 1.0f;

	tes_out.normal = texture(uni_normalTexture, texCoord).xyz;

	// tangent to world space
	tes_out.normal.xyz = tes_out.normal.xzy;

	vec4 localSpacePos = vec4(position.x, height, position.z, 1.0f);

	tes_out.positionWS = localSpacePos.xyz;

	vec4 thefrag_CameraSpacePos_current = uni_r_camera * uni_t_camera * localSpacePos;
	vec4 thefrag_CameraSpacePos_previous = uni_r_camera_prev * uni_t_camera_prev * localSpacePos;
	
	tes_out.positionCS = uni_p_camera_original * thefrag_CameraSpacePos_current;
	tes_out.positionCS_prev = uni_p_camera_original * thefrag_CameraSpacePos_previous;

	gl_Position = uni_p_camera_jittered * thefrag_CameraSpacePos_current;
}