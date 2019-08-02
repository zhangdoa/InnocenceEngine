// Spherical-Gaussians
// ----------------------------------------------------------------------------
struct SG
{
	float3 amplitude;
	float3 axis;
	float sharpness;
};

float3 EvaluateSG(in SG sg, in float3 dir)
{
	float cosAngle = dot(dir, sg.axis);
	return sg.amplitude * exp(sg.sharpness * (cosAngle - 1.0f));
}

SG SGProduct(in SG x, in SG y)
{
	float3 um = (x.sharpness * x.axis + y.sharpness * y.axis) /
		(x.sharpness + y.sharpness);
	float umLength = length(um);
	float lm = x.sharpness + y.sharpness;

	SG res;
	res.axis = um * (1.0f / umLength);
	res.sharpness = lm * umLength;
	res.amplitude = x.amplitude * y.amplitude *
		exp(lm * (umLength - 1.0f));

	return res;
}

float3 SGIntegral(in SG sg)
{
	float expTerm = 1.0f - exp(-2.0f * sg.sharpness);
	return 2 * PI * (sg.amplitude / sg.sharpness) * expTerm;
}

float3 SGInnerProduct(in SG x, in SG y)
{
	float umLength = length(x.sharpness * x.axis +
		y.sharpness * y.axis);
	float3 expo = exp(umLength - x.sharpness - y.sharpness) *
		x.amplitude * y.amplitude;
	float other = 1.0f - exp(-2.0f * umLength);
	return (2.0f * PI * expo * other) / umLength;
}

float SGsharpnessFromThreshold(in float amplitude,
	in float epsilon,
	in float cosTheta)
{
	return (log(epsilon) - log(amplitude)) / (cosTheta - 1.0f);
}

float3 SGIrradianceFitted(in SG lightingLobe, in float3 normal)
{
	const float muDotN = dot(lightingLobe.axis, normal);
	const float lambda = lightingLobe.sharpness;

	const float c0 = 0.36f;
	const float c1 = 1.0f / (4.0f * c0);

	float eml = exp(-lambda);
	float em2l = eml * eml;
	float rl = rcp(lambda);

	float scale = 1.0f + 2.0f * em2l - rl;
	float bias = (eml - em2l) * rl - em2l;

	float x = sqrt(1.0f - scale);
	float x0 = c0 * muDotN;
	float x1 = c1 * x;

	float n = x0 + x1;

	float y = saturate(muDotN);
	if (abs(x0) <= x1)
		y = n * n / x;

	float result = scale * y + bias;

	return result * SGIntegral(lightingLobe);
}

float3 SGGetIlluminance(SG lightingLobe, float3 albedo, float metallic, float roughness, float3 F0, float3 N, float3 V, float3 L)
{
	float3 H = normalize(V + L);

	float NdotV = max(dot(N, V), 0.0);
	float LdotH = max(dot(L, H), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float3 brdf = CalcBRDF(albedo, metallic, roughness, F0, NdotV, LdotH, NdotH, NdotL);

	return SGIrradianceFitted(lightingLobe, N) * brdf * NdotL;
}

SG SphereLightToSG(in float3 lightDir, in float radius, in float3 intensity, in float dist)
{
	SG sg;

	float r2 = radius * radius;
	float d2 = dist * dist;

	float lne = -2.230258509299f; // ln(0.1)
	sg.axis = normalize(lightDir);
	sg.sharpness = (-lne * d2) / r2;
	sg.amplitude = intensity;

	return sg;
}

SG DirectionalLightToSG(in float3 lightDir, in float3 intensity)
{
	SG sg;

	float lne = -2.230258509299f; // ln(0.1)
	sg.axis = normalize(lightDir);
	sg.sharpness = -lne;
	sg.amplitude = intensity;

	return sg;
}