// ----------------------------------------------------------------------------
float PCF(vec3 projCoords, sampler2D shadowMap, vec2 texelSize, vec2 offset)
{
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;

	// PCF
	float shadow = 0.0;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowMap, offset + projCoords.xy / 2.0 + vec2(x, y) * texelSize).r;
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	return shadow;
}
// ----------------------------------------------------------------------------
float VSMKernel(vec4 shadowMapValue, float currentDepth)
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
float DirectionalLightVSM(float NdotL, vec3 projCoords, sampler2D shadowMap, vec2 texelSize, vec2 offset)
{
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;

	// VSM
	vec4 shadowMapValue = texture(shadowMap, offset + projCoords.xy / 2.0);
	float shadow = VSMKernel(shadowMapValue, currentDepth);
	return shadow;
}
// ----------------------------------------------------------------------------
float DirectionalLightShadow(vec3 fragPos)
{
	vec3 projCoords = vec3(0.0);
	float shadow = 0.0;
	vec2 textureSize = textureSize(uni_directionalLightShadowMap, 0);
	vec2 texelSize = 1.0 / textureSize;
	// one shadow texture stores 4 different split results in character "N" shape order from left-bottom corner
	// 1 -- 3
	// |    |
	// 0 -- 2
	vec2 offsetSize = vec2(0.5, 0.5);

	int splitIndex = NR_CSM_SPLITS;
	for (int i = 0; i < NR_CSM_SPLITS; i++)
	{
		if (fragPos.x >= uni_CSMs[i].AABBMin.x &&
			fragPos.y >= uni_CSMs[i].AABBMin.y &&
			fragPos.z >= uni_CSMs[i].AABBMin.z &&
			fragPos.x <= uni_CSMs[i].AABBMax.x &&
			fragPos.y <= uni_CSMs[i].AABBMax.y &&
			fragPos.z <= uni_CSMs[i].AABBMax.z)
		{
			splitIndex = i;
			break;
		}
	}

	vec2 offset;

	if (splitIndex == NR_CSM_SPLITS)
	{
		shadow = 0.0;
	}
	else
	{
		vec4 lightSpacePos = uni_CSMs[splitIndex].p * uni_CSMs[splitIndex].v * vec4(fragPos, 1.0f);
		lightSpacePos = lightSpacePos / lightSpacePos.w;
		projCoords = lightSpacePos.xyz;

		if (splitIndex == 0)
		{
			offset = vec2(0, 0);
		}
		else if (splitIndex == 1)
		{
			offset = vec2(0, offsetSize.y);
		}
		else if (splitIndex == 2)
		{
			offset = vec2(offsetSize.x, 0);
		}
		else if (splitIndex == 3)
		{
			offset = offsetSize;
		}
		else
		{
			shadow = 0.0;
		}
	}

	shadow = PCF(projCoords, uni_directionalLightShadowMap, texelSize, offset);

	return shadow;
}
// ----------------------------------------------------------------------------
float PointLightShadow(vec3 fragPos)
{
	vec3 fragToLight = fragPos - uni_pointLights[0].position.xyz;
	float currentDepth = length(fragToLight);
	float lightRadius = uni_pointLights[0].luminance.w;

	if (currentDepth > lightRadius)
	{
		return 0;
	}

	vec3 texCoord = normalize(fragToLight);
	vec4 shadowMapValue = texture(uni_pointLightShadowMap, texCoord);

	float shadow = VSMKernel(shadowMapValue, currentDepth);
	return shadow;
}
// ----------------------------------------------------------------------------
float linearDepth(float depthSample)
{
	float zLinear = zNear * zFar / (zFar + zNear - depthSample * (zFar - zNear));
	return zLinear;
}