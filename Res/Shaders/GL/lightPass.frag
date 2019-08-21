// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec2 TexCoords;

layout(location = 0) out vec4 uni_lightPassRT0;

layout(location = 0, binding = 0) uniform sampler2D uni_opaquePassRT0;
layout(location = 1, binding = 1) uniform sampler2D uni_opaquePassRT1;
layout(location = 2, binding = 2) uniform sampler2D uni_opaquePassRT2;
layout(location = 3, binding = 3) uniform sampler2D uni_opaquePassRT3;
layout(location = 4, binding = 4) uniform sampler2D uni_brdfLUT;
layout(location = 5, binding = 5) uniform sampler2D uni_brdfMSLUT;
layout(location = 6, binding = 6) uniform sampler2D uni_SSAOBlurPassRT0;
layout(location = 7, binding = 7) uniform sampler2DArray uni_sunShadow;

layout(std430, binding = 8) coherent buffer lightIndexListSSBOBlock
{
	uint data[];
} lightIndexListSSBO;

layout(location = 9, binding = 9) uniform sampler2D uni_lightGrid;

layout(location = 10, binding = 10) uniform sampler3D uni_IrradianceVolume;

#include "common/BRDF.glsl"
#include "common/shadowResolver.glsl"

// ----------------------------------------------------------------------------
void main()
{
	vec2 renderTargetSize = vec2(textureSize(uni_opaquePassRT0, 0));
	vec2 texelSize = 1.0 / renderTargetSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec4 GPassRT0 = texture(uni_opaquePassRT0, screenTexCoords);
	vec4 GPassRT1 = texture(uni_opaquePassRT1, screenTexCoords);
	vec4 GPassRT2 = texture(uni_opaquePassRT2, screenTexCoords);
	vec4 GPassRT3 = texture(uni_opaquePassRT3, screenTexCoords);

	//vec2 textureSize = textureSize(uni_depth, 0);
	//vec2 screenTexCoord = gl_FragCoord.xy / textureSize;
	//float depth = texture(uni_depth, screenTexCoord).r;
	//vec4 posCS = vec4(screenTexCoord.x * 2.0f - 1.0f, screenTexCoord.y * 2.0f - 1.0f, depth * 2.0f - 1.0f, 1.0f);
	//vec4 posVS = skyUBO.p_inv * posCS;
	//posVS /= posVS.w;
	//vec4 FragPos = skyUBO.v_inv * posVS;
	//vec3 FragPos = FragPos.rgb;

	vec3 FragPos = GPassRT0.rgb;
	vec3 Normal = GPassRT1.rgb;
	vec3 Albedo = GPassRT2.rgb;

	float Metallic = GPassRT0.a;
	float Roughness = GPassRT1.a;
	float safe_roughness = (Roughness + eps) / (1.0 + eps);
	float AO = GPassRT2.a;
	float SSAO = texture(uni_SSAOBlurPassRT0, screenTexCoords).x;
	AO *= SSAO;

	vec3 Lo = vec3(0.0);
	vec3 N = normalize(Normal);
	vec3 L;
	float NdotL;

#ifdef uni_drawCSMSplitedArea
	L = normalize(-sunUBO.data.direction.xyz);
	NdotL = max(dot(N, L), 0.0);

	Lo = vec3(NdotL);
	Lo *= 1 - SunShadowResolver(FragPos);

	int splitIndex = NR_CSM_SPLITS;
	for (int i = 0; i < NR_CSM_SPLITS; i++)
	{
		if (FragPos.x >= CSMUBO.data[i].AABBMin.x &&
			FragPos.y >= CSMUBO.data[i].AABBMin.y &&
			FragPos.z >= CSMUBO.data[i].AABBMin.z &&
			FragPos.x <= CSMUBO.data[i].AABBMax.x &&
			FragPos.y <= CSMUBO.data[i].AABBMax.y &&
			FragPos.z <= CSMUBO.data[i].AABBMax.z)
		{
			splitIndex = i;
			break;
		}
	}

	if (splitIndex == 0)
	{
		Lo.g = 0;
		Lo.b = 0;
	}
	else if (splitIndex == 1)
	{
		Lo.b = 0;
	}
	else if (splitIndex == 2)
	{
		Lo.r = 0;
		Lo.b = 0;
	}
	else if (splitIndex == 3)
	{
		Lo.r = 0;
		Lo.g = 0;
	}
#endif

#ifdef uni_drawPointLightShadow
	pointLight light = pointLightUBO.data[0];

	float lightRadius = light.luminance.w;
	if (lightRadius > 0)
	{
		vec3 unormalizedL = light.position.xyz - FragPos;

		if (length(unormalizedL) < lightRadius)
		{
			L = normalize(unormalizedL);
			NdotL = max(dot(N, L), 0.0);

			float attenuation = 1.0;
			float invSqrAttRadius = 1.0 / max(lightRadius * lightRadius, eps);
			attenuation *= getDistanceAtt(unormalizedL, invSqrAttRadius);

			vec3 lightLuminance = light.luminance.xyz * vec3(NdotL) * attenuation;

			Lo = lightLuminance;
		}
	}
	Lo *= 1 - PointLightShadow(FragPos);
#endif
#if !defined (uni_drawCSMSplitedArea) && !defined (uni_drawPointLightShadow)
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, Albedo, Metallic);

	vec3 V = normalize(cameraUBO.globalPos.xyz - FragPos);

	float NdotV = max(dot(N, V), 0.0);

	// direction light, sun light
	L = normalize(-sunUBO.data.direction.xyz);
	vec3 H = normalize(V + L);

	float LdotH = max(dot(L, H), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	NdotL = max(dot(N, L), 0.0);

	Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, safe_roughness, Metallic, F0, Albedo, sunUBO.data.luminance.xyz);

	Lo *= 1.0 - SunShadowResolver(FragPos);

	// point punctual light
	// Get the index of the current pixel in the light grid.
	ivec2 tileIndex = ivec2(floor(gl_FragCoord.xy / BLOCK_SIZE));

	// Get the start position and offset of the light in the light index list.
	vec4 lightGrid = texture(uni_lightGrid, tileIndex);
	uint startOffset = floatBitsToUint(lightGrid.x);
	uint lightCount = floatBitsToUint(lightGrid.y);

	//for (int i = 0; i < lightCount; ++i)
	for (int i = 0; i < NR_POINT_LIGHTS; ++i)
	{
		//uint lightIndex = lightIndexListSSBO.data[startOffset + i];
		uint lightIndex = i;
		pointLight light = pointLightUBO.data[lightIndex];

		float lightRadius = light.luminance.w;
		if (lightRadius > 0)
		{
			vec3 unormalizedL = light.position.xyz - FragPos;

			if (length(unormalizedL) < lightRadius)
			{
				L = normalize(unormalizedL);
				H = normalize(V + L);

				LdotH = max(dot(L, H), 0.0);
				NdotH = max(dot(N, H), 0.0);
				NdotL = max(dot(N, L), 0.0);

				float attenuation = 1.0;
				float invSqrAttRadius = 1.0 / max(lightRadius * lightRadius, eps);
				attenuation *= getDistanceAtt(unormalizedL, invSqrAttRadius);

				vec3 lightLuminance = light.luminance.xyz * attenuation;

				Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, safe_roughness, Metallic, F0, Albedo, lightLuminance);
			}
		}
	}

	//Lo *= 1 - PointLightShadow(FragPos);

	// sphere area light
	for (int i = 0; i < NR_SPHERE_LIGHTS; ++i)
	{
		float lightRadius = sphereLightUBO.data[i].luminance.w;
		if (lightRadius > 0)
		{
			vec3 unormalizedL = sphereLightUBO.data[i].position.xyz - FragPos;
			L = normalize(unormalizedL);
			H = normalize(V + L);

			LdotH = max(dot(L, H), 0.0);
			NdotH = max(dot(N, H), 0.0);
			NdotL = max(dot(N, L), 0.0);

			float sqrDist = dot(unormalizedL, unormalizedL);

			float Beta = acos(NdotL);
			float H2 = sqrt(sqrDist);
			float h = H2 / lightRadius;
			float x = sqrt(max(h * h - 1, eps));
			float y = -x * (1 / tan(Beta));
			y = clamp(y, -1.0, 1.0);
			float illuminance = 0;

			if (h * cos(Beta) > 1)
			{
				illuminance = cos(Beta) / (h * h);
			}
			else
			{
				illuminance = (1 / max(PI * h * h, eps))
					* (cos(Beta) * acos(y) - x * sin(Beta) * sqrt(max(1 - y * y, eps)))
					+ (1 / PI) * atan((sin(Beta) * sqrt(max(1 - y * y, eps)) / x));
			}
			illuminance *= PI;

			Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, safe_roughness, Metallic, F0, Albedo, illuminance * sphereLightUBO.data[i].luminance.xyz);
		}
	}

	// GI
	// [https://steamcdn-a.akamaihd.net/apps/valve/2006/SIGGRAPH06_Course_ShadingInValvesSourceEngine.pdf]
	vec3 nSquared = N * N;
	ivec3 isNegative = ivec3(int(N.x < 0.0), int(N.y < 0.0), int(N.z < 0.0));
	vec3 GISampleCoord = FragPos / skyUBO.posWSNormalizer.xyz;
	ivec3 isOutside = ivec3(int(GISampleCoord.x > 1.0), int(GISampleCoord.y > 1.0), int(GISampleCoord.z > 1.0));

	if ((isOutside.x == 0) && (isOutside.y == 0) && (isOutside.z == 0))
	{
		vec3 GISampleCoordPX = GISampleCoord;
		vec3 GISampleCoordNX = GISampleCoord + vec3(0, 0, 1);
		vec3 GISampleCoordPY = GISampleCoord + vec3(0, 0, 2);
		vec3 GISampleCoordNY = GISampleCoord + vec3(0, 0, 3);
		vec3 GISampleCoordPZ = GISampleCoord + vec3(0, 0, 4);
		vec3 GISampleCoordNZ = GISampleCoord + vec3(0, 0, 5);

		vec4 indirectLight = vec4(0.0f, 0.0f, 0.0f, 0.0f);
		if ((isNegative.x == 1))
		{
			indirectLight.xyz += nSquared.x * texture(uni_IrradianceVolume, GISampleCoordNX).xyz;
		}
		else
		{
			indirectLight.xyz += nSquared.x * texture(uni_IrradianceVolume, GISampleCoordPX).xyz;
		}
		if ((isNegative.y == 1))
		{
			indirectLight.xyz += nSquared.y * texture(uni_IrradianceVolume, GISampleCoordNY).xyz;
		}
		else
		{
			indirectLight.xyz += nSquared.y * texture(uni_IrradianceVolume, GISampleCoordPY).xyz;
		}
		if ((isNegative.z == 1))
		{
			indirectLight.xyz += nSquared.z * texture(uni_IrradianceVolume, GISampleCoordNZ).xyz;
		}
		else
		{
			indirectLight.xyz += nSquared.z * texture(uni_IrradianceVolume, GISampleCoordPZ).xyz;
		}

		Lo += (1 - Metallic) * indirectLight.xyz;
	}

	// ambient occlusion
	Lo *= AO;
#endif
	uni_lightPassRT0.rgb = Lo;
	uni_lightPassRT0.a = 1.0;
}