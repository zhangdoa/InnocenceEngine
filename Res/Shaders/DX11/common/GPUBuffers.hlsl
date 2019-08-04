cbuffer cameraCBuffer : register(b0)
{
	matrix cam_p_original;
	matrix cam_p_jittered;
	matrix cam_r;
	matrix cam_t;
	matrix cam_r_prev;
	matrix cam_t_prev;
	float4 cam_globalPos;
	float cam_WHRatio;
	float cam_zNear;
	float cam_zFar;
};

cbuffer meshCBuffer : register(b1)
{
	matrix m;
	matrix m_prev;
	matrix normalMat;
	float UUID;
};

cbuffer materialCBuffer : register(b2)
{
	float4 albedo;
	float4 MRAT;
	bool useNormalTexture;
	bool useAlbedoTexture;
	bool useMetallicTexture;
	bool useRoughnessTexture;
	bool useAOTexture;
	bool material_padding1;
	bool material_padding2;
	bool material_padding3;
};

cbuffer sunCBuffer : register(b3)
{
	float4 dirLight_dir;
	float4 dirLight_luminance;
	matrix dirLight_r;
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
	matrix p_inv;
	matrix v_inv;
	float2 viewportSize;
	float2 sky_padding1;
};

cbuffer dispatchParamsCBuffer : register(b8)
{
	uint3 numThreadGroups;
	uint dispatchParamsCBuffer_padding1;
	uint3 numThreads;
	uint  dispatchParamsCBuffer_padding2;
}

cbuffer SH9CBuffer : register(b9)
{
	SH9 SH9s[64];
};

cbuffer SSAOKernelCBuffer : register(b10)
{
	float4 kernels[64];
}