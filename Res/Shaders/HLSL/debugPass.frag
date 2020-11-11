// shadertype=hlsl
#include "common/common.hlsl"

struct DebugMaterialData
{
	float4 color;
};

[[vk::binding(1, 1)]]
StructuredBuffer<DebugMaterialData> debugMaterialSBuffer : register(t1);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	uint materialID : MATERIALID;
};

struct PixelOutputType
{
	float4 debugPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	output.debugPassRT0 = debugMaterialSBuffer[input.materialID].color;

	return output;
}