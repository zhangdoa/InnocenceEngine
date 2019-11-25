// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(location = 0, set = 1, binding = 0) uniform texture2D uni_basePassRT0;
layout(location = 1, set = 1, binding = 1) uniform texture2D uni_billboardPassRT0;
layout(location = 2, set = 1, binding = 2) uniform texture2D uni_debugPassRT0;
layout(std430, set = 1, binding = 3) coherent buffer averageSSBOBlock
{
	float data[];
} averageSSBO;

layout(set = 2, binding = 0) uniform sampler samplerLinear;

const mat3 RGB2XYZ = mat3(
	0.4124564, 0.3575761, 0.1804375,
	0.2126729, 0.7151522, 0.0721750,
	0.0193339, 0.1191920, 0.9503041
);

const mat3 XYZ2RGB = mat3(
	3.2404542, -1.5371385, -0.4985314,
	-0.9692660, 1.8760108, 0.0415560,
	0.0556434, -0.2040259, 1.0572252
);

vec3 rgb2xyz(vec3 rgb)
{
	return RGB2XYZ * rgb;
}

vec3 xyz2rgb(vec3 xyz)
{
	return XYZ2RGB * xyz;
}

vec3 xyz2xyY(vec3 xyz)
{
	float Y = xyz.y;
	float x = xyz.x / (xyz.x + xyz.y + xyz.z);
	float y = xyz.y / (xyz.x + xyz.y + xyz.z);
	return vec3(x, y, Y);
}

vec3 xyY2xyz(vec3 xyY)
{
	float Y = xyY.z;
	float x = Y * xyY.x / xyY.y;
	float z = Y * (1.0 - xyY.x - xyY.y) / xyY.y;
	return vec3(x, Y, z);
}

vec3 rgb2xyY(vec3 rgb)
{
	vec3 xyz = rgb2xyz(rgb);
	return xyz2xyY(xyz);
}

vec3 xyY2rgb(vec3 xyY)
{
	vec3 xyz = xyY2xyz(xyY);
	return xyz2rgb(xyz);
}

// Academy Color Encoding System [http://www.oscars.org/science-technology/sci-tech-projects/aces]
vec3 acesFilm(const vec3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

//gamma correction with respect to human eyes non-linearity
//[https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
vec3 accurateLinearToSRGB(in vec3 linearCol)
{
	vec3 sRGBLo = linearCol * 12.92;
	vec3 sRGBHi = (pow(abs(linearCol), vec3(1.0 / 2.4)) * 1.055) - 0.055;

	bvec3 lessThanEqualResult = lessThanEqual(linearCol, vec3(0.0031308));
	vec3 sRGB = (lessThanEqualResult.x && lessThanEqualResult.y && lessThanEqualResult.z) ? sRGBLo : sRGBHi;
	return sRGB;
}

void main()
{
	vec2 texelSize = 1.0 / textureSize(uni_basePassRT0, 0);
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec3 basePass = texture(sampler2D(uni_basePassRT0, samplerLinear), screenTexCoords).rgb;
	vec3 billboardPass = texture(sampler2D(uni_billboardPassRT0, samplerLinear), screenTexCoords).rgb;
	vec4 debugPass = texture(sampler2D(uni_debugPassRT0, samplerLinear), screenTexCoords);

	// HDR to LDR
	vec3 bassPassxyY = rgb2xyY(basePass);
	bassPassxyY.z /= averageSSBO.data[0] * 9.6;
	basePass = xyY2rgb(bassPassxyY);

	vec3 finalColor = basePass / (1.0 + basePass);

	//color filter
	finalColor = acesFilm(finalColor);

	// gamma correction
	finalColor = accurateLinearToSRGB(finalColor);

	// billboard overlay
	finalColor += billboardPass;

	// debug overlay
	if (debugPass.a == 1.0)
	{
		finalColor = debugPass.rgb;
	}

	FragColor = vec4(finalColor, 1.0);
}