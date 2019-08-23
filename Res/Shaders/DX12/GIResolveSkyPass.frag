// shadertype=hlsl
#include "common/common.hlsl"
#include "common/skyResolver.hlsl"

struct PixelInputType
{
	float4 frag_ClipSpacePos : SV_POSITION;
	uint rtvId : SV_RenderTargetArrayIndex;
};

struct PixelOutputType
{
	float4 skyPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	float3 color = float3(0.0, 0.0, 0.0);

	float3 eyedir = get_world_normal(input.frag_ClipSpacePos.xy, sky_viewportSize.zw, GISky_p_inv, GISky_v_inv[input.rtvId]);

	if (eyedir.y > -0.1)
	{
		float3 lightdir = -sun_dir.xyz;
		float planetRadius = 6371e3;
		float atmosphereHeight = 100e3;
		float3 eye_position = float3(0.0, planetRadius, 0.0);

		color = atmosphere(
			eyedir,           // normalized ray direction
			eye_position,               // ray origin
			lightdir,                        // position of the sun
			22.0,                           // intensity of the sun
			planetRadius,                   // radius of the planet in meters
			planetRadius + atmosphereHeight, // radius of the atmosphere in meters
			float3(5.8e-6, 13.5e-6, 33.1e-6), // Rayleigh scattering coefficient
			21e-6,                          // Mie scattering coefficient
			8e3,                            // Rayleigh scale height
			1.3e3,                          // Mie scale height
			0.758                           // Mie preferred scattering direction
		);
	}

	output.skyPassRT0 = float4(color, 1.0);

	return output;
}