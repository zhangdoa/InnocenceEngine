// ----------------------------------------------------------------------------
float PCFResolver(float3 projCoords, Texture2DArray shadowMap, int index, float currentDepth, float2 texelSize)
{
	// PCF
	float shadow = 0.0;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = shadowMap.Sample(SampleTypePoint, float3(projCoords.xy + float2(x, y) * texelSize, index)).r;
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
float VSMResolver(float3 projCoords, Texture2DArray shadowMap, int index, float currentDepth)
{
	// VSM
	float4 shadowMapValue = shadowMap.Sample(SampleTypePoint, float3(projCoords.xy, index));

	float shadow = VSMKernel(shadowMapValue, currentDepth);
	return shadow;
}
// ----------------------------------------------------------------------------
float SunShadowResolver(float3 fragPos)
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

		// get depth of current fragment from light's perspective
		float currentDepth = projCoords.z;

		shadow = PCFResolver(projCoords, in_SunShadow, splitIndex, currentDepth, texelSize);
	}

	return shadow;
}
// ----------------------------------------------------------------------------
float linearDepth(float depthSample)
{
	float zLinear = cam_zNear * cam_zFar / (cam_zFar + cam_zNear - depthSample * (cam_zFar - cam_zNear));
	return zLinear;
}