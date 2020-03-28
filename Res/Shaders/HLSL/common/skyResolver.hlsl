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
	return (3.0 / (16.0 * PI)) * (1 + pow(cosTheta, 2.0));
}

// Henyey-Greenstein
float mie_HG(float cosTheta, float g)
{
	float g2 = pow(g, 2.0);
	float nom = 1.0 - g2;
	float denom = 4 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5) + 0.0001;
	return nom / denom;
}

// Schlick approximation
float mie_Schlick(float cosTheta, float g)
{
	float k = 1.55 * g - 0.55 * pow(g, 3.0);
	float nom = 1.0 - pow(k, 2.0);
	float denom = 4 * PI * pow((1.0 + k * cosTheta), 2.0) + 0.0001;
	return nom / denom;
}

//[https://www.alanzucconi.com/2017/10/10/atmospheric-scattering-3/]
float3 rayleigh_coeff(float3 rgb)
{
	return rgb * float3(0.00000519673, 0.0000121427, 0.0000296453);
}

#define iSteps 16
#define jSteps 8

float2 raySphereIntersection(float3 eyePos, float3 rayDir, float shpereRadius)
{
	// ray-sphere intersection that assumes
	// the sphere is centered at the origin.
	// No intersection when result.x > result.y
	float a = dot(rayDir, rayDir);
	float b = 2.0 * dot(rayDir, eyePos);
	float c = dot(eyePos, eyePos) - (shpereRadius * shpereRadius);
	float d = (b * b) - 4.0 * a * c;
	if (d < 0.0) return float2(1e5, -1e5);
	return float2(
		(-b - sqrt(d)) / (2.0 * a),
		(-b + sqrt(d)) / (2.0 * a)
		);
}

//[https://github.com/wwwtyro/glsl-atmosphere]
float3 atmosphere(float3 eyeDir, float3 eyePos, float3 sunPos, float sunIntensity, float planetRadius, float atmosphereRadius, float3 kRlh, float kMie, float shRlh, float shMie, float g)
{
	sunPos = normalize(sunPos);
	eyeDir = normalize(eyeDir);

	// Calculate the step size of the primary ray.
	float2 p = raySphereIntersection(eyePos, eyeDir, atmosphereRadius);
	if (p.x > p.y)
	{
		return float3(0, 0, 0);
	}

	p.y = min(p.y, raySphereIntersection(eyePos, eyeDir, planetRadius).x);
	float iStepSize = (p.y - p.x) / float(iSteps);

	// Initialize the primary ray time.
	float iTime = 0.0;

	// Initialize accumulators for Rayleigh and Mie scattering.
	float3 rayleigh_collected = float3(0, 0, 0);
	float3 mie_collected = float3(0, 0, 0);

	// Initialize optical depth accumulators for the primary ray.
	float iOdRlh = 0.0;
	float iOdMie = 0.0;

	// Calculate the Rayleigh and Mie phases.
	float cosTheta = dot(eyeDir, sunPos);
	float pRlh = rayleigh(cosTheta);
	float pMie = mie_Schlick(cosTheta, g);

	// Sample the primary ray.
	for (int i = 0; i < iSteps; i++)
	{
		// Calculate the primary ray sample position.
		float3 iPos = eyePos + eyeDir * (iTime + iStepSize * 0.5);

		// Calculate the height of the sample.
		float iHeight = length(iPos) - planetRadius;

		// Calculate the optical depth of the Rayleigh and Mie scattering for this step.
		float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
		float odStepMie = exp(-iHeight / shMie) * iStepSize;

		// Accumulate optical depth.
		iOdRlh += odStepRlh;
		iOdMie += odStepMie;

		// Calculate the step size of the secondary ray.
		float jStepSize = raySphereIntersection(iPos, sunPos, atmosphereRadius).y / float(jSteps);

		// Initialize the secondary ray time.
		float jTime = 0.0;

		// Initialize optical depth accumulators for the secondary ray.
		float jOdRlh = 0.0;
		float jOdMie = 0.0;

		// Sample the secondary ray.
		for (int j = 0; j < jSteps; j++)
		{
			// Calculate the secondary ray sample position.
			float3 jPos = iPos + sunPos * (jTime + jStepSize * 0.5);

			// Calculate the height of the sample.
			float jHeight = length(jPos) - planetRadius;

			// Accumulate the optical depth.
			jOdRlh += exp(-jHeight / shRlh) * jStepSize;
			jOdMie += exp(-jHeight / shMie) * jStepSize;

			// Increment the secondary ray time.
			jTime += jStepSize;
		}

		// Calculate attenuation.
		float3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

		// Accumulate scattering.
		rayleigh_collected += odStepRlh * attn;
		mie_collected += odStepMie * attn;

		// Increment the primary ray time.
		iTime += iStepSize;
	}

	// Calculate and return the final color.
	return sunIntensity * (pRlh * kRlh * rayleigh_collected + pMie * kMie * mie_collected);
}