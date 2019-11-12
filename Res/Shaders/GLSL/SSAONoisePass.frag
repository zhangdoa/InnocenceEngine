// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_SSAOPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, set = 1, binding = 0) uniform texture2D uni_Position;
layout(location = 1, set = 1, binding = 1) uniform texture2D uni_Normal;
layout(location = 2, set = 1, binding = 2) uniform texture2D uni_randomRot;

layout(set = 2, binding = 0) uniform sampler samplerLinear;
layout(set = 2, binding = 1) uniform sampler samplerWrap;

const float radius = 0.5f;
const float bias = 0.05f;

void main()
{
	vec2 screenTextureSize = textureSize(uni_Position, 0);
	vec2 texelSize = 1.0 / screenTextureSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec2 noiseScale = screenTextureSize / vec2(textureSize(uni_randomRot, 0));
	vec3 randomRot = texture(sampler2D(uni_randomRot, samplerWrap), screenTexCoords * noiseScale).xyz;

	// alpha channel is used previously, remove its unwanted influence
	// world space position to view space
	vec3 fragPos = texture(sampler2D(uni_Position, samplerLinear), screenTexCoords).xyz;
	fragPos = (cameraUBO.r * cameraUBO.t * vec4(fragPos, 1.0f)).xyz;

	// world space normal to view space
	vec3 normal = texture(sampler2D(uni_Normal, samplerLinear), screenTexCoords).xyz;
	normal = (cameraUBO.r * vec4(normal, 0.0f)).xyz;
	normal = normalize(normal);

	// create TBN change-of-basis matrix: from tangent-space to view-space
	vec3 tangent = normalize(randomRot - normal * dot(randomRot, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0f;
	for (int i = 0; i < 64; ++i)
	{
		// get sample position
		vec3 randomHemisphereSampleDir = TBN * SSAOKernelUBO.data[i].xyz; // from tangent to view-space
		vec3 randomHemisphereSamplePos = fragPos + randomHemisphereSampleDir * radius;

		// project sample position (to sample texture) (to get position on screen/texture)
		vec4 randomFragSampleCoord = vec4(randomHemisphereSamplePos, 1.0f);
		randomFragSampleCoord = cameraUBO.p_jittered * randomFragSampleCoord; // from view to clip-space
		randomFragSampleCoord.xyz /= randomFragSampleCoord.w; // perspective divide
		randomFragSampleCoord.xyz = randomFragSampleCoord.xyz * 0.5f + 0.5f; // transform to range 0.0 - 1.0

		randomFragSampleCoord = clamp(randomFragSampleCoord, 0.0f, 1.0f);

		// get sample depth
		vec4 randomFragSamplePos = texture(sampler2D(uni_Position, samplerLinear), randomFragSampleCoord.xy);

		// alpha channel is used previously, remove its unwanted influence
		randomFragSamplePos.w = 1.0f;
		randomFragSamplePos = cameraUBO.r * cameraUBO.t * randomFragSamplePos;

		// range check & accumulate
		float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - randomFragSamplePos.z), 0.0001f));
		occlusion += (randomFragSamplePos.z > randomHemisphereSamplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / float(64));

	uni_SSAOPassRT0 = vec4(vec3(occlusion), 1.0);
}