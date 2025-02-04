#define useCSM

// ----------------------------------------------------------------------------
float PCFResolver(float3 projCoords, Texture2DArray shadowMap, SamplerState in_sampler, int index, float currentDepth, float2 texelSize)
{
	// PCF
	float shadow = 0.0;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float3 coord = float3(projCoords.xy + float2(x, y) * texelSize, (float)index);
			float4 shadowSample = shadowMap.SampleLevel(in_sampler, coord, 0);
			float pcfDepth = shadowSample.r + 0.0005;
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
	variance = max(variance, EPSILON);
	float mD = currentDepth - Ex;
	float pMax = variance / (variance + (mD * mD));
	shadow = max(pMax, float(currentDepth >= Ex));

	return shadow;
}
// ----------------------------------------------------------------------------
float VSMResolver(float3 projCoords, Texture2DArray shadowMap, SamplerState in_sampler, int index, float currentDepth)
{
	// VSM
	float4 shadowMapValue = shadowMap.SampleLevel(in_sampler, float3(projCoords.xy, index), 0);

	float shadow = VSMKernel(shadowMapValue, currentDepth);
	return shadow;
}

#ifdef useCSM
// ----------------------------------------------------------------------------
float SunShadowResolver(float3 fragPos, Texture2DArray shadowMap, SamplerState in_sampler)
{
	float shadow = 0.0;

	int splitIndex = NR_CSM_SPLITS;
	[unroll]
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
		shadowMap.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, elements, level);
		float2 texelSize = 1.0 / renderTargetSize;

		float4 lightSpacePos = mul(float4(fragPos, 1.0f), CSMs[splitIndex].v);
		lightSpacePos = mul(lightSpacePos, CSMs[splitIndex].p);
		lightSpacePos = lightSpacePos / lightSpacePos.w;
		float3 projCoords = lightSpacePos.xyz;
		if (projCoords.x > 1.0 || projCoords.x < -1.0 || projCoords.y > 1.0 || projCoords.y < -1.0 || projCoords.z > 1.0 || projCoords.z < -1.0)
		{
			return 0.0;
		}

		// transform to [0,1] range
		projCoords = projCoords * 0.5 + 0.5;
		projCoords.y = 1.0 - projCoords.y;

		// get depth of current fragment from light's perspective
		float currentDepth = projCoords.z;

		//shadow = VSMResolver(projCoords, shadowMap, in_sampler, splitIndex, currentDepth);
		shadow = PCFResolver(projCoords, shadowMap, in_sampler, splitIndex, currentDepth, texelSize);
	}

	return shadow;
}
#else
float SunShadowResolver(float3 fragPos, Texture2DArray shadowMap, SamplerState in_sampler)
{
	float shadow = 0.0;

	float2 renderTargetSize;
	float level;
	float elements;
	shadowMap.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, elements, level);
	float2 texelSize = 1.0 / renderTargetSize;

	float4 lightSpacePos = mul(float4(fragPos, 1.0f), CSMs[0].v);
	lightSpacePos = mul(lightSpacePos, CSMs[0].p);
	lightSpacePos = lightSpacePos / lightSpacePos.w;

	float3 projCoords = lightSpacePos.xyz;
	if (projCoords.x > 1.0 || projCoords.x < -1.0 || projCoords.y > 1.0 || projCoords.y < -1.0 || projCoords.z > 1.0 || projCoords.z < -1.0)
	{
		return 0.0;
	}

	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	projCoords.y = 1.0 - projCoords.y;

	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;

	shadow = VSMResolver(projCoords, shadowMap, in_sampler, 0, currentDepth);

	return shadow;
}
#endif
// ----------------------------------------------------------------------------
float linearDepth(float depthSample)
{
	float zLinear = g_Frame.zNear * g_Frame.zFar / (g_Frame.zFar + g_Frame.zNear - depthSample * (g_Frame.zFar - g_Frame.zNear));
	return zLinear;
}