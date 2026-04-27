// shadertype=hlsl
//
// Cube-shadow caster — GS fan-out per shadow-casting point/sphere light × 6
// cube faces. Uses GS instancing (`[instance(NR_POINT_SHADOWS)]`) so one
// pipeline draw produces NR_POINT_SHADOWS × 6 face writes per input triangle.
//
// The instance ID is the atlas's per-light slot; `g_PointShadows[instanceID]`
// carries that light's per-face view matrices and atlasBaseSlot. Inactive
// slots short-circuit before any vertex emit so GS bandwidth scales with
// active casters, not the budget cap.
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float3 posWS : POSITION0;
	float2 texCoord : TEXCOORD;
	uint lightSlot : TEXCOORD1;
	uint rtvId : SV_RenderTargetArrayIndex;
};

[[vk::binding(1, 0)]]
cbuffer PointShadowCBuffer : register(b1)
{
	PointShadow_CB g_PointShadows[NR_POINT_SHADOWS];
};

// Per-instance-per-input-triangle vertex budget: 6 faces × 3 verts = 18.
// `[instance(N)]` produces N invocations of this body per input triangle, so
// the total emit per triangle is 18 × NR_POINT_SHADOWS in the worst case.
// Inactive lights early-out without emitting, keeping GS bandwidth proportional
// to live casters.
[instance(NR_POINT_SHADOWS)]
[maxvertexcount(18)]
void main(triangle GeometryInputType input[3], uint instanceID : SV_GSInstanceID, inout TriangleStream<PixelInputType> outStream)
{
	if (g_PointShadows[instanceID].isActive == 0)
		return;

	PointShadow_CB l_Light = g_PointShadows[instanceID];
	uint l_BaseSlot = l_Light.atlasBaseSlot;

	[unroll(6)]
	for (uint face = 0; face < 6; face++)
	{
		[unroll(3)]
		for (int i = 0; i < 3; i++)
		{
			PixelInputType output;
			float4 posV = mul(input[i].posWS, l_Light.v[face]);
			output.posCS = mul(posV, l_Light.p);
			output.posWS = input[i].posWS.xyz;
			output.texCoord = input[i].texCoord;
			output.lightSlot = instanceID;
			output.rtvId = l_BaseSlot + face;
			outStream.Append(output);
		}
		outStream.RestartStrip();
	}
}
