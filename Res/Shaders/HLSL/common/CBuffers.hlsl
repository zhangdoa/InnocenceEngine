cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(2, 0)]]
cbuffer pointLightCBuffer : register(b2)
{
	PointLight_CB pointLights[NR_POINT_LIGHTS];
};

[[vk::binding(3, 0)]]
cbuffer sphereLightCBuffer : register(b3)
{
	SphereLight_CB sphereLights[NR_SPHERE_LIGHTS];
};

cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

StructuredBuffer<PerObject_CB> g_Objects : register(t0);
StructuredBuffer<Material_CB>  g_Materials : register(t1);
Texture2D g_2DTextures[] : register(t2);
SamplerState g_Samplers[] : register(s0);

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