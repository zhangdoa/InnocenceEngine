// shadertype=hlsl

Texture2D in_opaquePassRT3 : register(t0);
Texture2D in_TAAPassRT0 : register(t1);

SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 motionBlurPassRT0 : SV_Target0;
};

static int MAX_SAMPLES = 16;

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;
	float2 renderTargetSize;
	float level;
	in_TAAPassRT0.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, level);
	float2 texelSize = 1.0 / renderTargetSize;
	float2 screenTexCoords = input.position.xy * texelSize;
	float2 MotionVector = in_opaquePassRT3.Sample(SampleTypePoint, screenTexCoords).xy;

	if (MotionVector.x == 0.0 && MotionVector.y == 0.0)
	{
		output.motionBlurPassRT0 = in_TAAPassRT0.Sample(SampleTypePoint, screenTexCoords);
	}
	else
	{
		float4 result = in_TAAPassRT0.Sample(SampleTypePoint, screenTexCoords);

		float half_samples = float(MAX_SAMPLES / 2);

		// sample half samples along motion vector and another half in opposite direction
		for (int i = 1; i <= half_samples; i++) {
			float2 offset = MotionVector * (float(i) / float(MAX_SAMPLES));
			result += in_TAAPassRT0.Sample(SampleTypePoint, screenTexCoords - offset);
			result += in_TAAPassRT0.Sample(SampleTypePoint, screenTexCoords + offset);
		}

		result /= float(MAX_SAMPLES);

		//use alpha channel as mask
		output.motionBlurPassRT0 = float4(result.rgb, 1.0);
	}

	return output;
}