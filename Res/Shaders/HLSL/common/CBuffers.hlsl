cbuffer perFrameCBuffer : register(b0)
{
	PerFrame_CB perFrameCBuffer;
};

cbuffer perObjectCBuffer : register(b1)
{
	PerObject_CB perObjectCBuffer;
};

cbuffer materialCBuffer : register(b2)
{
	Material_CB materialCBuffer;
};

cbuffer pointLightCBuffer : register(b3)
{
	PointLight_CB pointLights[NR_POINT_LIGHTS];
};

cbuffer sphereLightCBuffer : register(b4)
{
	SphereLight_CB sphereLights[NR_SPHERE_LIGHTS];
};

cbuffer CSMCBuffer : register(b5)
{
	CSM_CB CSMs[NR_CSM_SPLITS];
};

cbuffer dispatchParamsCBuffer : register(b6)
{
	DispatchParam_CB dispatchParams[8];
}

cbuffer SSAOKernelCBuffer : register(b7)
{
	float4 SSAOKernels[64];
};

cbuffer GICBuffer : register(b8)
{
	GI_CB GICBuffer;
};

cbuffer voxelizationPassCBuffer : register(b9)
{
	VoxelizationPass_CB voxelizationPassCBuffer;
};