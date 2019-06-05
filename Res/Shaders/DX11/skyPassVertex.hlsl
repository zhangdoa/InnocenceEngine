// shadertype=hlsl

cbuffer cameraCBuffer : register(b0)
{
	matrix cam_p_original;
	matrix cam_p_jittered;
	matrix cam_r;
	matrix cam_t;
	matrix cam_r_prev;
	matrix cam_t_prev;
	float4 cam_globalPos;
	float cam_WHRatio;
};

cbuffer meshCBuffer : register(b1)
{
	matrix m;
	matrix m_prev;
	matrix normalMat;
	float UUID;
};

struct VertexInputType
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	float2 pada : PADA;
	float4 normal : NORMAL;
	float4 padb : PADB;
};

struct PixelInputType
{
	float4 frag_ClipSpacePos : SV_POSITION;
	float3 frag_TexCoord : TEXCOORD;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.frag_TexCoord = input.position.xyz * -1.0;
	float4 pos = float4(-1.0 * input.position.xyz, 1.0);
	pos = mul(pos, cam_r);
	pos = mul(pos, cam_p_original);
	pos.z = pos.w;
	output.frag_ClipSpacePos = pos;

	return output;
}