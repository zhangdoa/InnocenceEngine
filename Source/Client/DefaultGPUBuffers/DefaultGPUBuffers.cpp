#include "DefaultGPUBuffers.h"
#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

namespace DefaultGPUBuffers
{
	GPUBufferDataComponent* m_PerFrameCBufferGBDC;
	GPUBufferDataComponent* m_MeshGBDC;
	GPUBufferDataComponent* m_MaterialGBDC;
	GPUBufferDataComponent* m_VolumetricFogPassMeshGBDC;
	GPUBufferDataComponent* m_VolumetricFogPassMaterialGBDC;
	GPUBufferDataComponent* m_PointLightGBDC;
	GPUBufferDataComponent* m_SphereLightGBDC;
	GPUBufferDataComponent* m_CSMGBDC;
	GPUBufferDataComponent* m_dispatchParamsGBDC;
	GPUBufferDataComponent* m_GICBufferGBDC;
	GPUBufferDataComponent* m_billboardGBDC;

	std::vector<DispatchParamsConstantBuffer> m_DispatchParamsConstantBuffer;
}

bool DefaultGPUBuffers::Setup()
{
	return true;
}

bool DefaultGPUBuffers::Initialize()
{
	auto l_RenderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_PerFrameCBufferGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("PerFrameCBuffer/");
	m_PerFrameCBufferGBDC->m_ElementCount = 1;
	m_PerFrameCBufferGBDC->m_ElementSize = sizeof(PerFrameConstantBuffer);
	m_PerFrameCBufferGBDC->m_BindingPoint = 0;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_PerFrameCBufferGBDC);

	m_MeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("PerObjectCBuffer/");
	m_MeshGBDC->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_MeshGBDC->m_ElementSize = sizeof(PerObjectConstantBuffer);
	m_MeshGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_MeshGBDC);

	m_MaterialGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("MaterialCBuffer/");
	m_MaterialGBDC->m_ElementCount = l_RenderingCapability.maxMaterials;
	m_MaterialGBDC->m_ElementSize = sizeof(MaterialConstantBuffer);
	m_MaterialGBDC->m_BindingPoint = 2;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_MaterialGBDC);

	m_VolumetricFogPassMeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VolumetricFogPassPerObjectCBuffer/");
	m_VolumetricFogPassMeshGBDC->m_ElementCount = 512;
	m_VolumetricFogPassMeshGBDC->m_ElementSize = sizeof(PerObjectConstantBuffer);
	m_VolumetricFogPassMeshGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_VolumetricFogPassMeshGBDC);

	m_VolumetricFogPassMaterialGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VolumetricFogPassMaterialCBuffer/");
	m_VolumetricFogPassMaterialGBDC->m_ElementCount = 512;
	m_VolumetricFogPassMaterialGBDC->m_ElementSize = sizeof(MaterialConstantBuffer);
	m_VolumetricFogPassMaterialGBDC->m_BindingPoint = 2;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_VolumetricFogPassMaterialGBDC);

	m_PointLightGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("PointLightCBuffer/");
	m_PointLightGBDC->m_ElementCount = l_RenderingCapability.maxPointLights;
	m_PointLightGBDC->m_ElementSize = sizeof(PointLightConstantBuffer);
	m_PointLightGBDC->m_BindingPoint = 4;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_PointLightGBDC);

	m_SphereLightGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SphereLightCBuffer/");
	m_SphereLightGBDC->m_ElementCount = l_RenderingCapability.maxSphereLights;
	m_SphereLightGBDC->m_ElementSize = sizeof(SphereLightConstantBuffer);
	m_SphereLightGBDC->m_BindingPoint = 5;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_SphereLightGBDC);

	m_CSMGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("CSMCBuffer/");
	m_CSMGBDC->m_ElementCount = l_RenderingCapability.maxCSMSplits;
	m_CSMGBDC->m_ElementSize = sizeof(CSMConstantBuffer);
	m_CSMGBDC->m_BindingPoint = 6;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_CSMGBDC);

	// @TODO: get rid of hard-code stuffs
	m_dispatchParamsGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("DispatchParamsCBuffer/");
	m_dispatchParamsGBDC->m_ElementCount = 8;
	m_dispatchParamsGBDC->m_ElementSize = sizeof(DispatchParamsConstantBuffer);
	m_dispatchParamsGBDC->m_BindingPoint = 8;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_dispatchParamsGBDC);

	m_GICBufferGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("GICBuffer/");
	m_GICBufferGBDC->m_ElementSize = sizeof(GIConstantBuffer);
	m_GICBufferGBDC->m_ElementCount = 1;
	m_GICBufferGBDC->m_BindingPoint = 10;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_GICBufferGBDC);

	m_billboardGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("BillboardCBuffer/");
	m_billboardGBDC->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_billboardGBDC->m_ElementSize = sizeof(PerObjectConstantBuffer);
	m_billboardGBDC->m_BindingPoint = 12;
	m_billboardGBDC->m_GPUAccessibility = Accessibility::ReadWrite;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_billboardGBDC);

	return true;
}

bool DefaultGPUBuffers::Upload()
{
	auto l_PerFrameConstantBuffer = g_pModuleManager->getRenderingFrontend()->getPerFrameConstantBuffer();
	auto& l_PerObjectConstantBuffer = g_pModuleManager->getRenderingFrontend()->getPerObjectConstantBuffer();
	auto l_TotalDrawCallCount = l_PerObjectConstantBuffer.size();
	auto& l_MaterialConstantBuffer = g_pModuleManager->getRenderingFrontend()->getMaterialConstantBuffer();
	auto& l_PointLightConstantBuffer = g_pModuleManager->getRenderingFrontend()->getPointLightConstantBuffer();
	auto& l_SphereLightConstantBuffer = g_pModuleManager->getRenderingFrontend()->getSphereLightConstantBuffer();
	auto& l_CSMConstantBuffer = g_pModuleManager->getRenderingFrontend()->getCSMConstantBuffer();
	auto& l_billboardPassPerObjectConstantBuffer = g_pModuleManager->getRenderingFrontend()->getBillboardPassPerObjectConstantBuffer();

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_PerFrameCBufferGBDC, &l_PerFrameConstantBuffer);

	if (l_PerObjectConstantBuffer.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_MeshGBDC, l_PerObjectConstantBuffer, 0, l_TotalDrawCallCount);
	}
	if (l_MaterialConstantBuffer.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_MaterialGBDC, l_MaterialConstantBuffer, 0, l_TotalDrawCallCount);
	}
	if (l_PointLightConstantBuffer.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_PointLightGBDC, l_PointLightConstantBuffer, 0, l_PointLightConstantBuffer.size());
	}
	if (l_SphereLightConstantBuffer.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_SphereLightGBDC, l_SphereLightConstantBuffer, 0, l_SphereLightConstantBuffer.size());
	}
	if (l_CSMConstantBuffer.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_CSMGBDC, l_CSMConstantBuffer);
	}
	if (l_billboardPassPerObjectConstantBuffer.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_billboardGBDC, l_billboardPassPerObjectConstantBuffer, 0, l_billboardPassPerObjectConstantBuffer.size());
	}

	return true;
}

bool DefaultGPUBuffers::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_PerFrameCBufferGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_MeshGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_MaterialGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_PointLightGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_SphereLightGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_CSMGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_dispatchParamsGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_GICBufferGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_billboardGBDC);

	return true;
}

GPUBufferDataComponent * DefaultGPUBuffers::GetGPUBufferDataComponent(GPUBufferUsageType usageType)
{
	GPUBufferDataComponent* l_result;

	switch (usageType)
	{
	case GPUBufferUsageType::PerFrame: l_result = m_PerFrameCBufferGBDC;
		break;
	case GPUBufferUsageType::Mesh: l_result = m_MeshGBDC;
		break;
	case GPUBufferUsageType::Material: l_result = m_MaterialGBDC;
		break;
	case GPUBufferUsageType::VolumetricFogPassMesh: l_result = m_VolumetricFogPassMeshGBDC;
		break;
	case GPUBufferUsageType::VolumetricFogPassMaterial: l_result = m_VolumetricFogPassMaterialGBDC;
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
	case GPUBufferUsageType::Billboard: l_result = m_billboardGBDC;
		break;
	default:
		break;
	}

	return l_result;
}