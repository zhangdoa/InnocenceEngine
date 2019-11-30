// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_skyPassRT0;

layout(location = 0) in vec3 TexCoords;

#include "common/skyResolver.glsl"

void main()
{
	vec3 color = vec3(0.0);

	vec3 eyedir = get_world_normal(gl_FragCoord.xy, perFrameCBuffer.data.viewportSize.xy, perFrameCBuffer.data.p_inv, perFrameCBuffer.data.v_inv);
	vec3 lightdir = -perFrameCBuffer.data.sun_direction.xyz;
	float planetRadius = 6371e3;
	float atmosphereHeight = 100e3;
	vec3 eye_position = perFrameCBuffer.data.camera_posWS.xyz + vec3(0.0, planetRadius, 0.0);

	color = atmosphere(
		eyedir,           // normalized ray direction
		eye_position,               // ray origin
		lightdir,                        // position of the sun
		22.0,                           // intensity of the sun
		planetRadius,                   // radius of the planet in meters
		planetRadius + atmosphereHeight, // radius of the atmosphere in meters
		vec3(5.8e-6, 13.5e-6, 33.1e-6), // Rayleigh scattering coefficient
		21e-6,                          // Mie scattering coefficient
		8e3,                            // Rayleigh scale height
		1.3e3,                          // Mie scale height
		0.758                           // Mie preferred scattering direction
	);
	uni_skyPassRT0 = vec4(color, 1.0);
}