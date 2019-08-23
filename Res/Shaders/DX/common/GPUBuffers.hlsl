cbuffer cameraCBuffer : register(b0)
{
	cameraData cameraCBuffer;
};

cbuffer meshCBuffer : register(b1)
{
	meshData meshCBuffer;
};

cbuffer materialCBuffer : register(b2)
{
	materialData materialCBuffer;
};

cbuffer sunCBuffer : register(b3)
{
	float4 sun_dir;
	float4 sun_luminance;
	matrix sun_r;
	float4 sun_padding[2];
};

cbuffer pointLightCBuffer : register(b4)
{
	pointLight pointLights[NR_POINT_LIGHTS];
};

cbuffer sphereLightCBuffer : register(b5)
{
	sphereLight sphereLights[NR_SPHERE_LIGHTS];
};

cbuffer CSMCBuffer : register(b6)
{
	CSM CSMs[NR_CSM_SPLITS];
};

cbuffer skyCBuffer : register(b7)
{
	matrix sky_p_inv;
	matrix sky_v_inv;
	float4 sky_viewportSize;
	float4 sky_posWSNormalizer;
	float4 sky_padding[6];
};

cbuffer dispatchParamsCBuffer : register(b8)
{
	DispatchParam dispatchParams[8];
}

cbuffer SSAOKernelCBuffer : register(b9)
{
	float4 SSAOKernels[64];
};

cbuffer GICameraCBuffer : register(b10)
{
	matrix GICamera_p;
	matrix GICamera_r[6];
	matrix GICamera_t;
};

cbuffer GISkyCBuffer : register(b11)
{
	matrix GISky_p_inv;
	matrix GISky_v_inv[6];
	float4 GISky_probeCount;
	float4 GISky_probeInterval;
	float4 GISky_workload;
	float4 GISky_irradianceVolumeOffset;
};