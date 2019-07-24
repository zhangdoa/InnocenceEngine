// shadertype=glsl
#include "common.glsl"
layout(location = 0) out vec4 uni_SSAOPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_Position;
layout(location = 1, binding = 1) uniform sampler2D uni_Normal;
layout(location = 2, binding = 2) uniform sampler2D uni_randomRot;

const int kernelSize = 64;

layout(location = 3) uniform vec4 uni_kernels[kernelSize];

float radius = 0.5f;
float bias = 0.05f;

void main()
{
	vec2 texelSize = 1.0 / uni_viewportSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec2 noiseScale = vec2(textureSize(uni_Position, 0)) / vec2(textureSize(uni_randomRot, 0));
	vec3 randomRot = texture(uni_randomRot, screenTexCoords * noiseScale).xyz;

	// alpha channel is used previously, remove its unwanted influence
	// world space position to view space
	vec3 fragPos = texture(uni_Position, screenTexCoords).xyz;
	fragPos = (uni_r_camera * uni_t_camera * vec4(fragPos, 1.0f)).xyz;

	// world space normal to view space
	vec3 normal = texture(uni_Normal, screenTexCoords).xyz;
	normal = (uni_r_camera * vec4(normal, 0.0f)).xyz;
	normal = normalize(normal);

	// create TBN change-of-basis matrix: from tangent-space to view-space
	vec3 tangent = normalize(randomRot - normal * dot(randomRot, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0f;
	for (int i = 0; i < kernelSize; ++i)
	{
		// get sample position
		vec3 randomHemisphereSampleDir = TBN * uni_kernels[i].xyz; // from tangent to view-space
		vec3 randomHemisphereSamplePos = fragPos + randomHemisphereSampleDir * radius;

		// project sample position (to sample texture) (to get position on screen/texture)
		vec4 randomFragSampleCoord = vec4(randomHemisphereSamplePos, 1.0f);
		randomFragSampleCoord = uni_p_camera_jittered * randomFragSampleCoord; // from view to clip-space
		randomFragSampleCoord.xyz /= randomFragSampleCoord.w; // perspective divide
		randomFragSampleCoord.xyz = randomFragSampleCoord.xyz * 0.5f + 0.5f; // transform to range 0.0 - 1.0

		// get sample depth
		vec4 randomFragSamplePos = texture(uni_Position, randomFragSampleCoord.xy);

		// alpha channel is used previously, remove its unwanted influence
		randomFragSamplePos.w = 1.0f;
		randomFragSamplePos = uni_r_camera * uni_t_camera * randomFragSamplePos;

		// range check & accumulate
		float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - randomFragSamplePos.z), 0.0001f));
		occlusion += (randomFragSamplePos.z > randomHemisphereSamplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / float(kernelSize));

	uni_SSAOPassRT0 = vec4(vec3(occlusion), 1.0);
}