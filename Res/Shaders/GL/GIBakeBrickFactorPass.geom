// shadertype=glsl
#include "common/common.glsl"
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in VS_OUT
{
	vec3 posWS;
	float UUID;
} gs_in[3];

layout(location = 0) out GS_OUT
{
	float distanceVS;
	float UUID;
} gs_out;

void main()
{
	for (int face = 0; face < 6; ++face)
	{
		gl_Layer = face;
		for (int i = 0; i < 3; ++i)
		{
			vec4 posVS = GICameraUBO.r[face] * GICameraUBO.t * vec4(gs_in[i].posWS, 1.0f);
			gs_out.distanceVS = length(posVS.xyz);
			gs_out.UUID = gs_in[i].UUID;
			gl_Position = GICameraUBO.p * posVS;
			EmitVertex();
		}
		EndPrimitive();
	}
}