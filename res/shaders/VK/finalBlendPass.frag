#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 outTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D basePassRT0;

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
	vec3 finalColor;
	vec4 basePassResult = texture(basePassRT0, outTexCoord);

	finalColor = basePassResult.rgb;

	// HDR to LDR
	finalColor = acesFilm(finalColor);

	// gamma correction
	finalColor = accurateLinearToSRGB(finalColor);

	outColor = vec4(finalColor, 1.0);
}