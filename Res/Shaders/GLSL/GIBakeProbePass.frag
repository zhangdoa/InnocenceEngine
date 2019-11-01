// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_GIProbePassRT0;

layout(location = 0) in VS_OUT
{
	vec4 posWS;
} fs_in;

void main()
{
	uni_GIProbePassRT0 = fs_in.posWS;
}