struct VertexInputType
{
	float3 posLS : POSITION;
	float3 normalLS : NORMAL;
	float3 tangentLS : TANGENT;
	float2 texCoord : TEXCOORD;
	float4 pad1 : PAD_A;
	uint instanceId : SV_InstanceID;
};

struct PixelInputType
{
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	return output;
}