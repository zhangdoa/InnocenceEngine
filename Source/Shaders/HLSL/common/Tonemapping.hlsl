
float3 TonemapReinhard(float3 color)
{
	return color / (1.0f + color);
}

// [http://graphicrants.blogspot.com/2013/12/tone-mapping.html]
float3 TonemapReinhardLuma(float3 color)
{
	float luma = GetLuma(color);
	return color / (1.0f + luma);
}

// [http://graphicrants.blogspot.com/2013/12/tone-mapping.html]
float3 TonemapInvertReinhardLuma(float3 color)
{
	float luma = GetLuma(color);
	return color / (1.0f - luma);
}

// [https://gpuopen.com/learn/optimized-reversible-tonemapper-for-resolve/]
float3 TonemapMax3(float3 color)
{
	float maxValue = max(max(color.x, color.y), color.z);
	return color / (1.0f + maxValue);
}

// [https://gpuopen.com/learn/optimized-reversible-tonemapper-for-resolve/]
float3 TonemapInvertMax3(float3 color)
{
	float maxValue = max(max(color.x, color.y), color.z);
	return color / (1.0f - maxValue);
}

// Academy Color Encoding System
// [http://www.oscars.org/science-technology/sci-tech-projects/aces]
float3 TonemapACES(const float3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// gamma correction with respect to human eyes non-linearity
// [https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
float3 AccurateLinearToSRGB(float3 linearCol)
{
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
	const float threshold = 0.0031308;
	const float3 threshold3 = float3(threshold, threshold, threshold);

	float sR = (linearCol.r <= threshold3.r) ? sRGBLo.r : sRGBHi.r;
	float sG = (linearCol.g <= threshold3.g) ? sRGBLo.g : sRGBHi.g;
	float sB = (linearCol.b <= threshold3.b) ? sRGBLo.b : sRGBHi.b;
	float3 sRGB = float3(sR, sG, sB);

	return sRGB;
}
