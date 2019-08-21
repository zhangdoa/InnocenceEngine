// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_GIBrickFactorPassRT0;

layout(location = 0) in GS_OUT
{
	float distanceVS;
	float UUID;
} fs_in;

void main()
{
	uni_GIBrickFactorPassRT0 = vec4(fs_in.distanceVS, fs_in.UUID, 0.0f, 0.0f);
}