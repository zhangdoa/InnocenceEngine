#include "DefaultGPUBuffers.h"
#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

namespace DefaultGPUBuffers
{
	GPUBufferDataComponent* m_PerFrameCBufferGBDC;
	GPUBufferDataComponent* m_MeshGBDC;
	GPUBufferDataComponent* m_MaterialGBDC;
	GPUBufferDataComponent* m_PointLightGBDC;
	GPUBufferDataComponent* m_SphereLightGBDC;
	GPUBufferDataComponent* m_CSMGBDC;
	GPUBufferDataComponent* m_dispatchParamsGBDC;
	GPUBufferDataComponent* m_GICBufferGBDC;
	GPUBufferDataComponent* m_animationGBDC;
	GPUBufferDataComponent* m_billboardGBDC;

	std::vector<DispatchParamsConstantBuffer> m_DispatchParamsConstantBuffer;
}

bool DefaultGPUBuffers::Setup()
{
	return true;
}

bool DefaultGPUBuffers::Initialize()
{
	auto l_RenderingCapability = g_Engine->getRenderingFrontend()->getRenderingCapability();

	m_PerFrameCBufferGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("PerFrameCBuffer/");
	m_PerFrameCBufferGBDC->m_ElementCount = 1;
	m_PerFrameCBufferGBDC->m_ElementSize = sizeof(PerFrameConstantBuffer);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_PerFrameCBufferGBDC);

	m_MeshGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("PerObjectCBuffer/");
	m_MeshGBDC->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_MeshGBDC->m_ElementSize = sizeof(PerObjectConstantBuffer);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_MeshGBDC);

	m_MaterialGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("MaterialCBuffer/");
	m_MaterialGBDC->m_ElementCount = l_RenderingCapability.maxMaterials;
	m_MaterialGBDC->m_ElementSize = sizeof(MaterialConstantBuffer);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_MaterialGBDC);

	m_PointLightGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("PointLightCBuffer/");
	m_PointLightGBDC->m_ElementCount = l_RenderingCapability.maxPointLights;
	m_PointLightGBDC->m_ElementSize = sizeof(PointLightConstantBuffer);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_PointLightGBDC);

	m_SphereLightGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("SphereLightCBuffer/");
	m_SphereLightGBDC->m_ElementCount = l_RenderingCapability.maxSphereLights;
	m_SphereLightGBDC->m_ElementSize = sizeof(SphereLightConstantBuffer);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_SphereLightGBDC);

	m_CSMGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("CSMCBuffer/");
	m_CSMGBDC->m_ElementCount = l_RenderingCapability.maxCSMSplits;
	m_CSMGBDC->m_ElementSize = sizeof(CSMConstantBuffer);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_CSMGBDC);

	// @TODO: get rid of hard-code stuffs
	m_dispatchParamsGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("DispatchParamsCBuffer/");
	m_dispatchParamsGBDC->m_ElementCount = 8;
	m_dispatchParamsGBDC->m_ElementSize = sizeof(DispatchParamsConstantBuffer);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_dispatchParamsGBDC);

	m_GICBufferGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("GICBuffer/");
	m_GICBufferGBDC->m_ElementSize = sizeof(GIConstantBuffer);
	m_GICBufferGBDC->m_ElementCount = 1;

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_GICBufferGBDC);

	m_animationGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("AnimationCBuffer/");
	m_animationGBDC->m_ElementCount = 512;
	m_animationGBDC->m_ElementSize = sizeof(AnimationConstantBuffer);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_animationGBDC);

	m_billboardGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("BillboardCBuffer/");
	m_billboardGBDC->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_billboardGBDC->m_ElementSize = sizeof(PerObjectConstantBuffer);
	m_billboardGBDC->m_GPUAccessibility = Accessibility::ReadWrite;

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_billboardGBDC);

	return true;
}

bool DefaultGPUBuffers::Upload()
{
	auto l_PerFrameConstantBuffer = g_Engine->getRenderingFrontend()->getPerFrameConstantBuffer();
	auto& l_PerObjectConstantBuffer = g_Engine->getRenderingFrontend()->getPerObjectConstantBuffer();
	auto l_TotalDrawCallCount = l_PerObjectConstantBuffer.size();
	auto& l_MaterialConstantBuffer = g_Engine->getRenderingFrontend()->getMaterialConstantBuffer();
	auto& l_PointLightConstantBuffer = g_Engine->getRenderingFrontend()->getPointLightConstantBuffer();
	auto& l_SphereLightConstantBuffer = g_Engine->getRenderingFrontend()->getSphereLightConstantBuffer();
	auto& l_CSMConstantBuffer = g_Engine->getRenderingFrontend()->getCSMConstantBuffer();
	auto& l_animationConstantBuffer = g_Engine->getRenderingFrontend()->getAnimationConstantBuffer();
	auto& l_billboardPassPerObjectConstantBuffer = g_Engine->getRenderingFrontend()->getBillboardPassPerObjectConstantBuffer();

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_PerFrameCBufferGBDC, &l_PerFrameConstantBuffer);

	if (l_PerObjectConstantBuffer.size() > 0)
	{
		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_MeshGBDC, l_PerObjectConstantBuffer, 0, l_TotalDrawCallCount);
	}
	if (l_MaterialConstantBuffer.size() > 0)
	{
		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_MaterialGBDC, l_MaterialConstantBuffer, 0, l_TotalDrawCallCount);
	}
	if (l_PointLightConstantBuffer.size() > 0)
	{
		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_PointLightGBDC, l_PointLightConstantBuffer, 0, l_PointLightConstantBuffer.size());
	}
	if (l_SphereLightConstantBuffer.size() > 0)
	{
		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_SphereLightGBDC, l_SphereLightConstantBuffer, 0, l_SphereLightConstantBuffer.size());
	}
	if (l_CSMConstantBuffer.size() > 0)
	{
		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_CSMGBDC, l_CSMConstantBuffer);
	}
	if (l_animationConstantBuffer.size() > 0)
	{
		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_animationGBDC, l_animationConstantBuffer);
	}
	if (l_billboardPassPerObjectConstantBuffer.size() > 0)
	{
		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_billboardGBDC, l_billboardPassPerObjectConstantBuffer, 0, l_billboardPassPerObjectConstantBuffer.size());
	}

	return true;
}

bool DefaultGPUBuffers::Terminate()
{
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_PerFrameCBufferGBDC);
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_MeshGBDC);
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_MaterialGBDC);
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_PointLightGBDC);
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_SphereLightGBDC);
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_CSMGBDC);
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_dispatchParamsGBDC);
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_GICBufferGBDC);
	g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_billboardGBDC);

	return true;
}

Inno::GPUBufferDataComponent* DefaultGPUBuffers::GetGPUBufferDataComponent(GPUBufferUsageType usageType)
{
	Inno::GPUBufferDataComponent* l_result;

	switch (usageType)
	{
	case GPUBufferUsageType::PerFrame: l_result = m_PerFrameCBufferGBDC;
		break;
	case GPUBufferUsageType::Mesh: l_result = m_MeshGBDC;
		break;
	case GPUBufferUsageType::Material: l_result = m_MaterialGBDC;
		break;
	case GPUBufferUsageType::PointLight: l_result = m_PointLightGBDC;
		break;
	case GPUBufferUsageType::SphereLight: l_result = m_SphereLightGBDC;
		break;
	case GPUBufferUsageType::CSM: l_result = m_CSMGBDC;
		break;
	case GPUBufferUsageType::ComputeDispatchParam: l_result = m_dispatchParamsGBDC;
		break;
	case GPUBufferUsageType::GI:l_result = m_GICBufferGBDC;
		break;
	case GPUBufferUsageType::Animation: l_result = m_animationGBDC;
		break;
	case GPUBufferUsageType::Billboard: l_result = m_billboardGBDC;
		break;
	default:
		break;
	}

	return l_result;
}