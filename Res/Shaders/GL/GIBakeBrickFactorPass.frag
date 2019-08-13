// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_GIBrickFactorPassRT0;

layout(location = 0) in GS_OUT
{
	vec4 posWS;
	float UUID;
} fs_in;

void main()
{
	uni_GIBrickFactorPassRT0 = vec4(fs_in.posWS.xyz, fs_in.UUID);
}