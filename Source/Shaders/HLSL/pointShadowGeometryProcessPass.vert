// shadertype=hlsl
//
// Point/sphere cube-shadow caster — VS pass.
// Mirrors sunShadowGeometryProcessPass.vert: passthrough world-space transform.
// The GS does the cube-face fan-out per active shadow-casting light and the
// PS writes packed linear distance to the per-face atlas slice.
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

[[vk::binding(0, 0)]]
cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(0, 1)]]
StructuredBuffer<Transform_CB> g_Transforms : register(t0);

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	Transform_CB transformCB = g_Transforms[m_ObjectIndex];

	// Engine matrices are row-major; HLSL default column-major sees the
	// transpose, so use row-vector × matrix order. Reference: skyResolver.hlsl.
	output.posWS = mul(float4(input.posLS, 1.0f), transformCB.m);
	output.texCoord = input.texCoord;

	return output;
}
