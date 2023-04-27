#include "DefaultGPUBuffers.h"
#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

namespace DefaultGPUBuffers
{
	GPUBufferComponent* m_PerFrameCBufferGPUBufferComp;
	GPUBufferComponent* m_MeshGPUBufferComp;
	GPUBufferComponent* m_MaterialGPUBufferComp;
	GPUBufferComponent* m_PointLightGPUBufferComp;
	GPUBufferComponent* m_SphereLightGPUBufferComp;
	GPUBufferComponent* m_CSMGPUBufferComp;
	GPUBufferComponent* m_dispatchParamsGPUBufferComp;
	GPUBufferComponent* m_GICBufferGPUBufferComp;
	GPUBufferComponent* m_animationGPUBufferComp;
	GPUBufferComponent* m_billboardGPUBufferComp;

	std::vector<DispatchParamsConstantBuffer> m_DispatchParamsConstantBuffer;
}

bool DefaultGPUBuffers::Setup()
{
	return true;
}

bool DefaultGPUBuffers::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	auto l_RenderingCapability = g_Engine->getRenderingFrontend()->GetRenderingCapability();

	m_PerFrameCBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PerFrameCBuffer/");
	m_PerFrameCBufferGPUBufferComp->m_ElementCount = 1;
	m_PerFrameCBufferGPUBufferComp->m_ElementSize = sizeof(PerFrameConstantBuffer);

	l_renderingServer->InitializeGPUBufferComponent(m_PerFrameCBufferGPUBufferComp);

	m_MeshGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PerObjectCBuffer/");
	m_MeshGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_MeshGPUBufferComp->m_ElementSize = sizeof(PerObjectConstantBuffer);

	l_renderingServer->InitializeGPUBufferComponent(m_MeshGPUBufferComp);

	m_MaterialGPUBufferComp = l_renderingServer->AddGPUBufferComponent("MaterialCBuffer/");
	m_MaterialGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMaterials;
	m_MaterialGPUBufferComp->m_ElementSize = sizeof(MaterialConstantBuffer);

	l_renderingServer->InitializeGPUBufferComponent(m_MaterialGPUBufferComp);

	m_PointLightGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PointLightCBuffer/");
	m_PointLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxPointLights;
	m_PointLightGPUBufferComp->m_ElementSize = sizeof(PointLightConstantBuffer);

	l_renderingServer->InitializeGPUBufferComponent(m_PointLightGPUBufferComp);

	m_SphereLightGPUBufferComp = l_renderingServer->AddGPUBufferComponent("SphereLightCBuffer/");
	m_SphereLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxSphereLights;
	m_SphereLightGPUBufferComp->m_ElementSize = sizeof(SphereLightConstantBuffer);

	l_renderingServer->InitializeGPUBufferComponent(m_SphereLightGPUBufferComp);

	m_CSMGPUBufferComp = l_renderingServer->AddGPUBufferComponent("CSMCBuffer/");
	m_CSMGPUBufferComp->m_ElementCount = l_RenderingCapability.maxCSMSplits;
	m_CSMGPUBufferComp->m_ElementSize = sizeof(CSMConstantBuffer);

	l_renderingServer->InitializeGPUBufferComponent(m_CSMGPUBufferComp);

	// @TODO: get rid of hard-code stuffs
	m_dispatchParamsGPUBufferComp = l_renderingServer->AddGPUBufferComponent("DispatchParamsCBuffer/");
	m_dispatchParamsGPUBufferComp->m_ElementCount = 8;
	m_dispatchParamsGPUBufferComp->m_ElementSize = sizeof(DispatchParamsConstantBuffer);

	l_renderingServer->InitializeGPUBufferComponent(m_dispatchParamsGPUBufferComp);

	m_GICBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("GICBuffer/");
	m_GICBufferGPUBufferComp->m_ElementSize = sizeof(GIConstantBuffer);
	m_GICBufferGPUBufferComp->m_ElementCount = 1;

	l_renderingServer->InitializeGPUBufferComponent(m_GICBufferGPUBufferComp);

	m_animationGPUBufferComp = l_renderingServer->AddGPUBufferComponent("AnimationCBuffer/");
	m_animationGPUBufferComp->m_ElementCount = 512;
	m_animationGPUBufferComp->m_ElementSize = sizeof(AnimationConstantBuffer);

	l_renderingServer->InitializeGPUBufferComponent(m_animationGPUBufferComp);

	m_billboardGPUBufferComp = l_renderingServer->AddGPUBufferComponent("BillboardCBuffer/");
	m_billboardGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_billboardGPUBufferComp->m_ElementSize = sizeof(PerObjectConstantBuffer);
	m_billboardGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	l_renderingServer->InitializeGPUBufferComponent(m_billboardGPUBufferComp);

	return true;
}

bool DefaultGPUBuffers::Upload()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameConstantBuffer = g_Engine->getRenderingFrontend()->GetPerFrameConstantBuffer();
	auto& l_PerObjectConstantBuffer = g_Engine->getRenderingFrontend()->GetPerObjectConstantBuffer();
	auto l_TotalDrawCallCount = l_PerObjectConstantBuffer.size();
	auto& l_MaterialConstantBuffer = g_Engine->getRenderingFrontend()->GetMaterialConstantBuffer();
	auto& l_PointLightConstantBuffer = g_Engine->getRenderingFrontend()->GetPointLightConstantBuffer();
	auto& l_SphereLightConstantBuffer = g_Engine->getRenderingFrontend()->GetSphereLightConstantBuffer();
	auto& l_CSMConstantBuffer = g_Engine->getRenderingFrontend()->GetCSMConstantBuffer();
	auto& l_animationConstantBuffer = g_Engine->getRenderingFrontend()->GetAnimationConstantBuffer();
	auto& l_billboardPassPerObjectConstantBuffer = g_Engine->getRenderingFrontend()->GetBillboardPassPerObjectConstantBuffer();

	l_renderingServer->UploadGPUBufferComponent(m_PerFrameCBufferGPUBufferComp, &l_PerFrameConstantBuffer);

	if (l_PerObjectConstantBuffer.size() > 0)
	{
		l_renderingServer->UploadGPUBufferComponent(m_MeshGPUBufferComp, l_PerObjectConstantBuffer, 0, l_TotalDrawCallCount);
	}
	if (l_MaterialConstantBuffer.size() > 0)
	{
		l_renderingServer->UploadGPUBufferComponent(m_MaterialGPUBufferComp, l_MaterialConstantBuffer, 0, l_TotalDrawCallCount);
	}
	if (l_PointLightConstantBuffer.size() > 0)
	{
		l_renderingServer->UploadGPUBufferComponent(m_PointLightGPUBufferComp, l_PointLightConstantBuffer, 0, l_PointLightConstantBuffer.size());
	}
	if (l_SphereLightConstantBuffer.size() > 0)
	{
		l_renderingServer->UploadGPUBufferComponent(m_SphereLightGPUBufferComp, l_SphereLightConstantBuffer, 0, l_SphereLightConstantBuffer.size());
	}
	if (l_CSMConstantBuffer.size() > 0)
	{
		l_renderingServer->UploadGPUBufferComponent(m_CSMGPUBufferComp, l_CSMConstantBuffer);
	}
	if (l_animationConstantBuffer.size() > 0)
	{
		l_renderingServer->UploadGPUBufferComponent(m_animationGPUBufferComp, l_animationConstantBuffer);
	}
	if (l_billboardPassPerObjectConstantBuffer.size() > 0)
	{
		l_renderingServer->UploadGPUBufferComponent(m_billboardGPUBufferComp, l_billboardPassPerObjectConstantBuffer, 0, l_billboardPassPerObjectConstantBuffer.size());
	}

	return true;
}

bool DefaultGPUBuffers::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteGPUBufferComponent(m_PerFrameCBufferGPUBufferComp);
	l_renderingServer->DeleteGPUBufferComponent(m_MeshGPUBufferComp);
	l_renderingServer->DeleteGPUBufferComponent(m_MaterialGPUBufferComp);
	l_renderingServer->DeleteGPUBufferComponent(m_PointLightGPUBufferComp);
	l_renderingServer->DeleteGPUBufferComponent(m_SphereLightGPUBufferComp);
	l_renderingServer->DeleteGPUBufferComponent(m_CSMGPUBufferComp);
	l_renderingServer->DeleteGPUBufferComponent(m_dispatchParamsGPUBufferComp);
	l_renderingServer->DeleteGPUBufferComponent(m_GICBufferGPUBufferComp);
	l_renderingServer->DeleteGPUBufferComponent(m_billboardGPUBufferComp);

	return true;
}

Inno::GPUBufferComponent* DefaultGPUBuffers::GetGPUBufferComponent(GPUBufferUsageType usageType)
{
	Inno::GPUBufferComponent* l_result;

	switch (usageType)
	{
	case GPUBufferUsageType::PerFrame: l_result = m_PerFrameCBufferGPUBufferComp;
		break;
	case GPUBufferUsageType::Mesh: l_result = m_MeshGPUBufferComp;
		break;
	case GPUBufferUsageType::Material: l_result = m_MaterialGPUBufferComp;
		break;
	case GPUBufferUsageType::PointLight: l_result = m_PointLightGPUBufferComp;
		break;
	case GPUBufferUsageType::SphereLight: l_result = m_SphereLightGPUBufferComp;
		break;
	case GPUBufferUsageType::CSM: l_result = m_CSMGPUBufferComp;
		break;
	case GPUBufferUsageType::ComputeDispatchParam: l_result = m_dispatchParamsGPUBufferComp;
		break;
	case GPUBufferUsageType::GI:l_result = m_GICBufferGPUBufferComp;
		break;
	case GPUBufferUsageType::Animation: l_result = m_animationGPUBufferComp;
		break;
	case GPUBufferUsageType::Billboard: l_result = m_billboardGPUBufferComp;
		break;
	default:
		break;
	}

	return l_result;
}