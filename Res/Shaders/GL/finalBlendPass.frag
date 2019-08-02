// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(location = 0, binding = 0) uniform sampler2D uni_basePassRT0;
layout(location = 1, binding = 1) uniform sampler2D uni_billboardPassRT0;
layout(location = 2, binding = 2) uniform sampler2D uni_debuggerPassRT0;

// Academy Color Encoding System [http://www.oscars.org/science-technology/sci-tech-projects/aces]
vec3 acesFilm(const vec3 x) {
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// gamma correction with respect to human eyes non-linearity [https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
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
	vec2 texelSize = 1.0 / uni_viewportSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec3 finalColor;
	vec4 basePassResult = texture(uni_basePassRT0, screenTexCoords);
	vec4 billboardPassResult = texture(uni_billboardPassRT0, screenTexCoords);
	vec4 debuggerPassResult = texture(uni_debuggerPassRT0, screenTexCoords);

	finalColor = basePassResult.rgb;

	// HDR to LDR
	finalColor = acesFilm(finalColor);

	// gamma correction
	finalColor = accurateLinearToSRGB(finalColor);

	// billboard overlay
	finalColor += billboardPassResult.rgb;

	// debugger overlay
	finalColor += debuggerPassResult.rgb;

	FragColor = vec4(finalColor, 1.0);
}