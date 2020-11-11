[[vk::binding(0, 0)]]
cbuffer perFrameCBuffer : register(b0)
{
	PerFrame_CB perFrameCBuffer;
};

[[vk::binding(1, 0)]]
cbuffer perObjectCBuffer : register(b1)
{
	PerObject_CB perObjectCBuffer;
};

[[vk::binding(2, 0)]]
cbuffer materialCBuffer : register(b2)
{
	Material_CB materialCBuffer;
};

[[vk::binding(3, 0)]]
cbuffer pointLightCBuffer : register(b3)
{
	PointLight_CB pointLights[NR_POINT_LIGHTS];
};

[[vk::binding(4, 0)]]
cbuffer sphereLightCBuffer : register(b4)
{
	SphereLight_CB sphereLights[NR_SPHERE_LIGHTS];
};

[[vk::binding(5, 0)]]
cbuffer CSMCBuffer : register(b5)
{
	CSM_CB CSMs[NR_CSM_SPLITS];
};

[[vk::binding(6, 0)]]
cbuffer dispatchParamsCBuffer : register(b6)
{
	DispatchParam_CB dispatchParams[8];
}

[[vk::binding(7, 0)]]
cbuffer SSAOKernelCBuffer : register(b7)
{
	float4 SSAOKernels[64];
};

[[vk::binding(8, 0)]]
cbuffer GICBuffer : register(b8)
{
	GI_CB GICBuffer;
};

[[vk::binding(9, 0)]]
cbuffer voxelizationPassCBuffer : register(b9)
{
	VoxelizationPass_CB voxelizationPassCBuffer;
};

[[vk::binding(10, 0)]]
cbuffer animationPassCBuffer : register(b10)
{
	AnimationPass_CB animationPassCBuffer;
};