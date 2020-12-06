struct VertexInputType
{
	float4 posLS : POSITION;
	float2 texCoord : TEXCOORD;
	float2 pada : PADA;
	float4 normalLS : NORMAL;
	float4 padb : PADB;
};

struct PixelInputType
{
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	return output;
}