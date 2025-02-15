float3 get_world_normal(float2 xy, float2 viewportSize, matrix p_inv, matrix v_inv)
{
	float2 frag_coord = xy / viewportSize;
	frag_coord = (frag_coord - 0.5) * 2.0;
	float4 device_normal = float4(frag_coord, 0.0, 1.0);
	float4 eye_normal = mul(device_normal, p_inv);
	eye_normal = eye_normal / eye_normal.w;
	eye_normal.w = 0.0;
	eye_normal = mul(eye_normal, v_inv);
	float3 world_normal = normalize(eye_normal.xyz);
	return world_normal;
}

float rayleigh(float cosTheta)
{
	return (3.0 / (16.0 * PI)) * (1 + cosTheta * cosTheta);
}

// Henyey-Greenstein Phase Function
float mie_HG(float cosTheta, float g)
{
	float g2 = g * g;
	float denom = 4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5) + EPSILON; // Avoid div/0
	return (1.0 - g2) / denom;
}

// Schlick Approximation for Mie Scattering
float mie_Schlick(float cosTheta, float g)
{
	float k = 1.55 * g - 0.55 * (g * g);
	float denom = 4.0 * PI * pow(1.0 - k * cosTheta, 2.0) + EPSILON; // Avoid div/0
	return (1.0 - k * k) / denom;
}

//[https://www.alanzucconi.com/2017/10/10/atmospheric-scattering-3/]
float3 rayleigh_coeff(float3 rgb)
{
	return rgb * float3(0.00000519673, 0.0000121427, 0.0000296453);
}

#define iStEPSILON 16
#define jStEPSILON 8

float2 raySphereIntersection(float3 eyePos, float3 rayDir, float sphereRadius)
{
	float a = dot(rayDir, rayDir);
	float b = 2.0 * dot(rayDir, eyePos);
	float c = dot(eyePos, eyePos) - (sphereRadius * sphereRadius);
	float d = (b * b) - 4.0 * a * c;

	// Avoid NaNs: If determinant is negative, return no intersection
	if (d < 0.0) return float2(EPSILON, EPSILON);

	float sqrtD = sqrt(d);
	float t0 = (-b - sqrtD) / (2.0 * a);
	float t1 = (-b + sqrtD) / (2.0 * a);

	// Ensure valid order of t-values
	return float2(min(t0, t1), max(t0, t1));
}

//[https://github.com/wwwtyro/glsl-atmosphere]
float3 atmosphere(float3 eyeDir, float3 eyePos, float3 sunPos, float3 sunIntensity, float planetRadius, float atmosphereRadius, float3 kRlh, float kMie, float shRlh, float shMie, float g)
{
	sunPos = normalize(sunPos);
	eyeDir = normalize(eyeDir);

	// Ray-Sphere Intersection with Fixes
	float2 p = raySphereIntersection(eyePos, eyeDir, atmosphereRadius);
	if (p.x > p.y) return float3(0, 0, 0);

	p.y = min(p.y, raySphereIntersection(eyePos, eyeDir, planetRadius).x);
	float iStepSize = (p.y - p.x) / float(iStEPSILON);

	float3 rayleigh_collected = 0;
	float3 mie_collected = 0;
	float iOdRlh = 0.0;
	float iOdMie = 0.0;

	float cosTheta = dot(eyeDir, sunPos);
	float pRlh = rayleigh(cosTheta);
	float pMie = mie_Schlick(cosTheta, g);

	float iTime = 0.0;
	for (int i = 0; i < iStEPSILON; i++)
	{
		float3 iPos = eyePos + eyeDir * (iTime + iStepSize * 0.5);
		float iHeight = max(length(iPos) - planetRadius, 0.0); // Avoid negatives

		float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
		float odStepMie = exp(-iHeight / shMie) * iStepSize;

		iOdRlh += odStepRlh;
		iOdMie += odStepMie;

		float jStepSize = raySphereIntersection(iPos, sunPos, atmosphereRadius).y / float(jStEPSILON);
		float jTime = 0.0;

		float jOdRlh = 0.0;
		float jOdMie = 0.0;
		for (int j = 0; j < jStEPSILON; j++)
		{
			float3 jPos = iPos + sunPos * (jTime + jStepSize * 0.5);
			float jHeight = max(length(jPos) - planetRadius, 0.0); // Avoid negatives

			jOdRlh += exp(-jHeight / shRlh) * jStepSize;
			jOdMie += exp(-jHeight / shMie) * jStepSize;

			jTime += jStepSize;
		}

		float3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));
		rayleigh_collected += odStepRlh * attn;
		mie_collected += odStepMie * attn;
		iTime += iStepSize;
	}

	// Prevent NaNs or extreme colors
	float3 finalColor = sunIntensity * (pRlh * kRlh * rayleigh_collected + pMie * kMie * mie_collected);
	finalColor = max(finalColor, 0.0); // Avoid negatives

	return finalColor;
}

float3 getSkyColor(float3 eyedir, float3 eye_position, float3 lightdir, float3 sun_illuminance, float planetRadius, float atmosphereHeight)
{
	return atmosphere(
		eyedir, // normalized ray direction
		eye_position, // ray origin
		lightdir, // position of the sun
		sun_illuminance, // intensity of the sun
		planetRadius, // radius of the planet in meters
		planetRadius + atmosphereHeight, // radius of the atmosphere in meters
		float3(5.8e-6, 13.5e-6, 33.1e-6), // Rayleigh scattering coefficient
		21e-6, // Mie scattering coefficient
		8e3, // Rayleigh scale height
		1.3e3, // Mie scale height
		0.758 // Mie preferred scattering direction
	);
}