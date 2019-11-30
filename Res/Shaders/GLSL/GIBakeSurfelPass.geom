// shadertype=glsl
#include "common/common.glsl"
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in VS_OUT
{
	vec4 posWS;
	vec2 texcoord;
	vec4 normal;
} gs_in[3];

layout(location = 0) out GS_OUT
{
	vec4 posWS;
	vec2 texcoord;
	vec4 normal;
} gs_out;

void main()
{
	for (int face = 0; face < 6; ++face)
	{
		gl_Layer = face;
		for (int i = 0; i < 3; ++i)
		{
			gs_out.posWS = gs_in[i].posWS;
			gs_out.texcoord = gs_in[i].texcoord;
			gs_out.normal = gs_in[i].normal;
			gl_Position = GICBuffer.p * GICBuffer.r[face] * GICBuffer.t * gs_out.posWS;
			EmitVertex();
		}
		EndPrimitive();
	}
}