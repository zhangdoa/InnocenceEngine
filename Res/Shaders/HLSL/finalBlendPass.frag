// shadertype=hlsl

Texture2D basePassRT0 : register(t0);
Texture2D billboardPassRT0 : register(t1);
Texture2D debugPassRT0 : register(t2);
StructuredBuffer<float> in_luminanceAverage : register(t3);

SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

static const float3x3 RGB2XYZ = float3x3(
	0.4124564, 0.3575761, 0.1804375,
	0.2126729, 0.7151522, 0.0721750,
	0.0193339, 0.1191920, 0.9503041
);

static const float3x3 XYZ2RGB = float3x3(
	3.2404542, -1.5371385, -0.4985314,
	-0.9692660, 1.8760108, 0.0415560,
	0.0556434, -0.2040259, 1.0572252
);

float3 rgb2xyz(float3 rgb)
{
	return mul(rgb, RGB2XYZ);
}

float3 xyz2rgb(float3 xyz)
{
	return mul(xyz, XYZ2RGB);
}

float3 xyz2xyY(float3 xyz)
{
	float Y = xyz.y;
	float x = xyz.x / (xyz.x + xyz.y + xyz.z);
	float y = xyz.y / (xyz.x + xyz.y + xyz.z);
	return float3(x, y, Y);
}

float3 xyY2xyz(float3 xyY)
{
	float Y = xyY.z;
	float x = Y * xyY.x / xyY.y;
	float z = Y * (1.0 - xyY.x - xyY.y) / xyY.y;
	return float3(x, Y, z);
}

float3 rgb2xyY(float3 rgb)
{
	float3 xyz = rgb2xyz(rgb);
	return xyz2xyY(xyz);
}

float3 xyY2rgb(float3 xyY)
{
	float3 xyz = xyY2xyz(xyY);
	return xyz2rgb(xyz);
}

//Academy Color Encoding System
//http://www.oscars.org/science-technology/sci-tech-projects/aces
float3 acesFilm(const float3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(PixelInputType input) : SV_TARGET
{
	float3 basePass = basePassRT0.Sample(SampleTypePoint, input.texcoord);
	float4 billboardPass = billboardPassRT0.Sample(SampleTypePoint, input.texcoord);
	float4 debugPass = debugPassRT0.Sample(SampleTypePoint, input.texcoord);

	//HDR to LDR
	float3 bassPassxyY = rgb2xyY(basePass);
	bassPassxyY.z /= in_luminanceAverage[0] * 9.6;
	basePass = xyY2rgb(bassPassxyY);

	float3 finalColor = basePass / (1.0 + basePass);

	//color filter
	finalColor = acesFilm(finalColor);

	//gamma correction
	float gammaRatio = 1.0 / 2.2;
	finalColor = pow(finalColor, gammaRatio);

	// billboard overlay
	finalColor += billboardPass.rgb;

	// debug overlay
	if (debugPass.a == 1.0)
	{
		finalColor = debugPass.rgb;
	}

	return float4(finalColor, 1.0f);
}