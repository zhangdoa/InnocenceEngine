// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 1)]]
RWStructuredBuffer<uint> out_geometryProcessResult : register(u0);
[[vk::binding(0, 2)]]
Texture2D t2d_normal : register(t0);
[[vk::binding(1, 2)]]
Texture2D t2d_albedo : register(t1);
[[vk::binding(2, 2)]]
Texture2D t2d_metallic : register(t2);
[[vk::binding(3, 2)]]
Texture2D t2d_roughness : register(t3);
[[vk::binding(4, 2)]]
Texture2D t2d_ao : register(t4);
[[vk::binding(5, 2)]]
Texture2DArray in_SunShadow : register(t5);
[[vk::binding(0, 3)]]
SamplerState in_samplerTypePoint : register(s0);

#include "common/shadowResolver.hlsl"

#include "common/BSDF.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION;
	float4 posWS : POS_WS;
	nointerpolation float4 AABB : AABB;
	float4 normalLS : NORMAL;
	float2 texCoord : TEXCOORD;
};

void imageAtomicRGBA8Avg(RWStructuredBuffer<uint> grid, int index, float4 value)
{
    value.rgb *= 255.0;
    uint newVal = Float4ToRGBA8(value);
    uint prevStoredVal = 1;
    uint curStoredVal = 0;
    uint numIterations = 0;

    while(curStoredVal != prevStoredVal && numIterations < 255)
    {
		InterlockedCompareExchange(grid[index], curStoredVal, newVal, prevStoredVal);
        float4 rval = RGBA8ToFloat4(curStoredVal);
        rval.rgb = (rval.rgb * rval.a); // Denormalize
        float4 curValF = rval + value;  // Add
        curValF.rgb /= curValF.a;       // Renormalize
        newVal = Float4ToRGBA8(curValF);
		curStoredVal = newVal;
		
        ++numIterations;
    }
}

void main(PixelInputType input)
{
	float3 powWS = input.posCS_orig.xyz;

	if ((powWS.x < -1.0) || (powWS.y < -1.0) || (powWS.z < -1.0) || (powWS.x > 1.0) || (powWS.y > 1.0) || (powWS.z > 1.0))
	{
		discard;
	}

	float transparency = 1.0;
	float4 out_Albedo = materialCBuffer.albedo;
	if (materialCBuffer.textureSlotMask & 0x00000002)
	{
		out_Albedo = t2d_albedo.Sample(in_samplerTypePoint, input.texCoord);
	}

	float out_Metallic;
	if (materialCBuffer.textureSlotMask & 0x00000004)
	{
		out_Metallic = t2d_metallic.Sample(in_samplerTypePoint, input.texCoord).r;
	}
	else
	{
		out_Metallic = materialCBuffer.MRAT.r;
	}

	float out_Roughness;
	if (materialCBuffer.textureSlotMask & 0x00000008)
	{
		out_Roughness = t2d_roughness.Sample(in_samplerTypePoint, input.texCoord).r;
	}
	else
	{
		out_Roughness = materialCBuffer.MRAT.g;
	}

	float3 writeCoord = (input.posCS_orig.xyz * 0.5 + 0.5) * voxelizationPassCBuffer.volumeResolution;
	int3 writeCoordInt = int3(writeCoord);
	int index = writeCoordInt.x + writeCoordInt.y * voxelizationPassCBuffer.volumeResolution + writeCoordInt.z * voxelizationPassCBuffer.volumeResolution * voxelizationPassCBuffer.volumeResolution;

	// @TODO: optimize
	int offset = voxelizationPassCBuffer.volumeResolution * voxelizationPassCBuffer.volumeResolution * voxelizationPassCBuffer.volumeResolution;

	imageAtomicRGBA8Avg(out_geometryProcessResult, index, out_Albedo);
	imageAtomicRGBA8Avg(out_geometryProcessResult, index + offset, input.normalLS);
}