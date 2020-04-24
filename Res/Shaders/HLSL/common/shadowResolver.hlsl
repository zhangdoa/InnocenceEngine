#define useCSM

// ----------------------------------------------------------------------------
float PCFResolver(float3 projCoords, Texture2DArray shadowMap, SamplerState Sampler, int index, float currentDepth, float2 texelSize)
{
	// PCF
	float shadow = 0.0;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float3 coord = float3(projCoords.xy + float2(x, y) * texelSize, (float)index);
			float4 shadowSample = shadowMap.SampleLevel(Sampler, coord, 0);
			float pcfDepth = shadowSample.r;
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	return shadow;
}
// ----------------------------------------------------------------------------
float VSMKernel(float4 shadowMapValue, float currentDepth)
{
	float shadow = 0.0;
	float Ex = shadowMapValue.r;
	float E_x2 = shadowMapValue.g;
	float variance = E_x2 - (Ex * Ex);
	float mD = Ex - currentDepth;
	float p = variance / (variance + mD * mD);

	shadow = max(p, float(currentDepth >= Ex));

	return shadow;
}
// ----------------------------------------------------------------------------
float VSMResolver(float3 projCoords, Texture2DArray shadowMap, SamplerState Sampler, int index, float currentDepth)
{
	// VSM
	float4 shadowMapValue = shadowMap.SampleLevel(Sampler, float3(projCoords.xy, index), 0);

	float shadow = VSMKernel(shadowMapValue, currentDepth);
	return shadow;
}

#ifdef useCSM
// ----------------------------------------------------------------------------
float SunShadowResolver(float3 fragPos, SamplerState Sampler)
{
	float shadow = 0.0;

	int splitIndex = NR_CSM_SPLITS;
	for (int i = 0; i < NR_CSM_SPLITS; i++)
	{
		if (fragPos.x >= CSMs[i].AABBMin.x &&
			fragPos.y >= CSMs[i].AABBMin.y &&
			fragPos.z >= CSMs[i].AABBMin.z &&
			fragPos.x <= CSMs[i].AABBMax.x &&
			fragPos.y <= CSMs[i].AABBMax.y &&
			fragPos.z <= CSMs[i].AABBMax.z)
		{
			splitIndex = i;
			break;
		}
	}

	if (splitIndex == NR_CSM_SPLITS)
	{
		shadow = 0.0;
	}
	else
	{
		float2 renderTargetSize;
		float level;
		float elements;
		in_SunShadow.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, elements, level);
		float2 texelSize = 1.0 / renderTargetSize;

		float4 lightSpacePos = mul(float4(fragPos, 1.0f), CSMs[splitIndex].v);
		lightSpacePos = mul(lightSpacePos, CSMs[splitIndex].p);
		lightSpacePos = lightSpacePos / lightSpacePos.w;
		float3 projCoords = lightSpacePos.xyz;

		// transform to [0,1] range
		projCoords = projCoords * 0.5 + 0.5;
		projCoords.y = 1.0 - projCoords.y;
		// get depth of current fragment from light's perspective
		float currentDepth = projCoords.z;

		shadow = PCFResolver(projCoords, in_SunShadow, Sampler, splitIndex, currentDepth, texelSize);
	}

	return shadow;
}
#else
float SunShadowResolver(float3 fragPos, SamplerState Sampler)
{
	float shadow = 0.0;

	float2 renderTargetSize;
	float level;
	float elements;
	in_SunShadow.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, elements, level);
	float2 texelSize = 1.0 / renderTargetSize;

	float4 lightSpacePos = mul(float4(fragPos, 1.0f), CSMs[0].v);
	lightSpacePos = mul(lightSpacePos, CSMs[0].p);
	lightSpacePos = lightSpacePos / lightSpacePos.w;
	float3 projCoords = lightSpacePos.xyz;

	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	projCoords.y = 1.0 - projCoords.y;
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;

	if (currentDepth > 1.0)
	{
		return 0.0;
	}

	shadow = PCFResolver(projCoords, in_SunShadow, Sampler, 0, currentDepth, texelSize);

	return shadow;
}
#endif
// ----------------------------------------------------------------------------
float linearDepth(float depthSample)
{
	float zLinear = perFrameCBuffer.zNear * perFrameCBuffer.zFar / (perFrameCBuffer.zFar + perFrameCBuffer.zNear - depthSample * (perFrameCBuffer.zFar - perFrameCBuffer.zNear));
	return zLinear;
}