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
	float4 dirLight_dir;
	float4 dirLight_luminance;
	matrix dirLight_r;
	float4 padding[2];
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
	float4 posWSNormalizer;
	float2 viewportSize;
	float4 sky_padding[6];
};

cbuffer dispatchParamsCBuffer : register(b8)
{
	DispatchParam dispatchParams[8];
}

cbuffer SH9CBuffer : register(b9)
{
	SH9 SH9s[64];
};

cbuffer SSAOKernelCBuffer : register(b10)
{
	float4 kernels[64];
};

cbuffer GICameraCBuffer : register(b11)
{
	matrix GI_cam_p;
	matrix GI_cam_r[6];
	matrix GI_cam_t;
};