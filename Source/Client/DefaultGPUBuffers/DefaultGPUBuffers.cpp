#include "DefaultGPUBuffers.h"
#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

namespace DefaultGPUBuffers
{
	GPUBufferDataComponent* m_MainCameraGBDC;
	GPUBufferDataComponent* m_SunShadowPassMeshGBDC;
	GPUBufferDataComponent* m_OpaquePassMeshGBDC;
	GPUBufferDataComponent* m_OpaquePassMaterialGBDC;
	GPUBufferDataComponent* m_TransparentPassMeshGBDC;
	GPUBufferDataComponent* m_TransparentPassMaterialGBDC;
	GPUBufferDataComponent* m_VolumetricFogPassMeshGBDC;
	GPUBufferDataComponent* m_VolumetricFogPassMaterialGBDC;
	GPUBufferDataComponent* m_SunGBDC;
	GPUBufferDataComponent* m_PointLightGBDC;
	GPUBufferDataComponent* m_SphereLightGBDC;
	GPUBufferDataComponent* m_CSMGBDC;
	GPUBufferDataComponent* m_SkyGBDC;
	GPUBufferDataComponent* m_dispatchParamsGBDC;
	GPUBufferDataComponent* m_GICameraGBDC;
	GPUBufferDataComponent* m_GISkyGBDC;
	GPUBufferDataComponent* m_billboardGBDC;

	std::vector<DispatchParamsGPUData> m_DispatchParamsGPUData;
}

bool DefaultGPUBuffers::Setup()
{
	return true;
}

bool DefaultGPUBuffers::Initialize()
{
	auto l_RenderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_MainCameraGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("CameraGPUBuffer/");
	m_MainCameraGBDC->m_ElementCount = 1;
	m_MainCameraGBDC->m_ElementSize = sizeof(CameraGPUData);
	m_MainCameraGBDC->m_BindingPoint = 0;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_MainCameraGBDC);

	m_SunShadowPassMeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SunShadowPassMeshGPUBuffer/");
	m_SunShadowPassMeshGBDC->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_SunShadowPassMeshGBDC->m_ElementSize = sizeof(MeshGPUData);
	m_SunShadowPassMeshGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_SunShadowPassMeshGBDC);

	m_OpaquePassMeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("OpaquePassMeshGPUBuffer/");
	m_OpaquePassMeshGBDC->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_OpaquePassMeshGBDC->m_ElementSize = sizeof(MeshGPUData);
	m_OpaquePassMeshGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_OpaquePassMeshGBDC);

	m_OpaquePassMaterialGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("OpaquePassMaterialGPUBuffer/");
	m_OpaquePassMaterialGBDC->m_ElementCount = l_RenderingCapability.maxMaterials;
	m_OpaquePassMaterialGBDC->m_ElementSize = sizeof(MaterialGPUData);
	m_OpaquePassMaterialGBDC->m_BindingPoint = 2;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_OpaquePassMaterialGBDC);

	m_TransparentPassMeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("TransparentPassMeshGPUBuffer/");
	m_TransparentPassMeshGBDC->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_TransparentPassMeshGBDC->m_ElementSize = sizeof(MeshGPUData);
	m_TransparentPassMeshGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_TransparentPassMeshGBDC);

	m_TransparentPassMaterialGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("TransparentPassMaterialGPUBuffer/");
	m_TransparentPassMaterialGBDC->m_ElementCount = l_RenderingCapability.maxMaterials;
	m_TransparentPassMaterialGBDC->m_ElementSize = sizeof(MaterialGPUData);
	m_TransparentPassMaterialGBDC->m_BindingPoint = 2;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_TransparentPassMaterialGBDC);

	m_VolumetricFogPassMeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VolumetricFogPassMeshGPUBuffer/");
	m_VolumetricFogPassMeshGBDC->m_ElementCount = 512;
	m_VolumetricFogPassMeshGBDC->m_ElementSize = sizeof(MeshGPUData);
	m_VolumetricFogPassMeshGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_VolumetricFogPassMeshGBDC);

	m_VolumetricFogPassMaterialGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VolumetricFogPassMaterialGPUBuffer/");
	m_VolumetricFogPassMaterialGBDC->m_ElementCount = 512;
	m_VolumetricFogPassMaterialGBDC->m_ElementSize = sizeof(MaterialGPUData);
	m_VolumetricFogPassMaterialGBDC->m_BindingPoint = 2;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_VolumetricFogPassMaterialGBDC);

	m_SunGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SunGPUBuffer/");
	m_SunGBDC->m_ElementCount = 1;
	m_SunGBDC->m_ElementSize = sizeof(SunGPUData);
	m_SunGBDC->m_BindingPoint = 3;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_SunGBDC);

	m_PointLightGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("PointLightGPUBuffer/");
	m_PointLightGBDC->m_ElementCount = l_RenderingCapability.maxPointLights;
	m_PointLightGBDC->m_ElementSize = sizeof(PointLightGPUData);
	m_PointLightGBDC->m_BindingPoint = 4;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_PointLightGBDC);

	m_SphereLightGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SphereLightGPUBuffer/");
	m_SphereLightGBDC->m_ElementCount = l_RenderingCapability.maxSphereLights;
	m_SphereLightGBDC->m_ElementSize = sizeof(SphereLightGPUData);
	m_SphereLightGBDC->m_BindingPoint = 5;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_SphereLightGBDC);

	m_CSMGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("CSMGPUBuffer/");
	m_CSMGBDC->m_ElementCount = l_RenderingCapability.maxCSMSplits;
	m_CSMGBDC->m_ElementSize = sizeof(CSMGPUData);
	m_CSMGBDC->m_BindingPoint = 6;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_CSMGBDC);

	m_SkyGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SkyGPUBuffer/");
	m_SkyGBDC->m_ElementCount = 1;
	m_SkyGBDC->m_ElementSize = sizeof(SkyGPUData);
	m_SkyGBDC->m_BindingPoint = 7;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_SkyGBDC);

	// @TODO: get rid of hard-code stuffs
	m_dispatchParamsGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("DispatchParamsGPUBuffer/");
	m_dispatchParamsGBDC->m_ElementCount = 8;
	m_dispatchParamsGBDC->m_ElementSize = sizeof(DispatchParamsGPUData);
	m_dispatchParamsGBDC->m_BindingPoint = 8;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_dispatchParamsGBDC);

	m_GICameraGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("GICameraGPUBuffer/");
	m_GICameraGBDC->m_ElementSize = sizeof(GICameraGPUData);
	m_GICameraGBDC->m_ElementCount = 1;
	m_GICameraGBDC->m_BindingPoint = 10;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_GICameraGBDC);

	m_GISkyGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("GISkyGPUBuffer/");
	m_GISkyGBDC->m_ElementSize = sizeof(GISkyGPUData);
	m_GISkyGBDC->m_ElementCount = 1;
	m_GISkyGBDC->m_BindingPoint = 11;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_GISkyGBDC);

	m_billboardGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("BillboardGPUBuffer/");
	m_billboardGBDC->m_ElementCount = l_RenderingCapability.maxMeshes;
	m_billboardGBDC->m_ElementSize = sizeof(MeshGPUData);
	m_billboardGBDC->m_BindingPoint = 12;
	m_billboardGBDC->m_GPUAccessibility = Accessibility::ReadWrite;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_billboardGBDC);

	return true;
}

bool DefaultGPUBuffers::Upload()
{
	auto l_CameraGPUData = g_pModuleManager->getRenderingFrontend()->getCameraGPUData();
	auto l_SunShadowPassTotalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getSunShadowPassDrawCallCount();
	auto& l_SunShadowPassMeshGPUData = g_pModuleManager->getRenderingFrontend()->getSunShadowPassMeshGPUData();
	auto l_OpaquePassTotalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	auto& l_OpaquePassMeshGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassMeshGPUData();
	auto& l_OpaquePassMaterialGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassMaterialGPUData();
	auto l_TransparentPassTotalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getTransparentPassDrawCallCount();
	auto& l_TransparentPassMeshGPUData = g_pModuleManager->getRenderingFrontend()->getTransparentPassMeshGPUData();
	auto& l_TransparentPassMaterialGPUData = g_pModuleManager->getRenderingFrontend()->getTransparentPassMaterialGPUData();
	auto& l_SunGPUData = g_pModuleManager->getRenderingFrontend()->getSunGPUData();
	auto& l_PointLightGPUData = g_pModuleManager->getRenderingFrontend()->getPointLightGPUData();
	auto& l_SphereLightGPUData = g_pModuleManager->getRenderingFrontend()->getSphereLightGPUData();
	auto& l_CSMGPUData = g_pModuleManager->getRenderingFrontend()->getCSMGPUData();
	auto l_SkyGPUData = g_pModuleManager->getRenderingFrontend()->getSkyGPUData();
	auto& l_billboardPassMeshGPUData = g_pModuleManager->getRenderingFrontend()->getBillboardPassMeshGPUData();

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_MainCameraGBDC, &l_CameraGPUData);
	if (l_SunShadowPassMeshGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_SunShadowPassMeshGBDC, l_SunShadowPassMeshGPUData, 0, l_SunShadowPassTotalDrawCallCount);
	}
	if (l_OpaquePassMeshGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_OpaquePassMeshGBDC, l_OpaquePassMeshGPUData, 0, l_OpaquePassTotalDrawCallCount);
	}
	if (l_OpaquePassMaterialGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_OpaquePassMaterialGBDC, l_OpaquePassMaterialGPUData, 0, l_OpaquePassTotalDrawCallCount);
	}
	if (l_TransparentPassMeshGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_TransparentPassMeshGBDC, l_TransparentPassMeshGPUData, 0, l_TransparentPassTotalDrawCallCount);
	}
	if (l_TransparentPassMaterialGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_TransparentPassMaterialGBDC, l_TransparentPassMaterialGPUData, 0, l_TransparentPassTotalDrawCallCount);
	}
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_SunGBDC, &l_SunGPUData);
	if (l_PointLightGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_PointLightGBDC, l_PointLightGPUData, 0, l_PointLightGPUData.size());
	}
	if (l_SphereLightGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_SphereLightGBDC, l_SphereLightGPUData, 0, l_SphereLightGPUData.size());
	}
	if (l_CSMGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_CSMGBDC, l_CSMGPUData);
	}
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_SkyGBDC, &l_SkyGPUData);

	if (l_billboardPassMeshGPUData.size() > 0)
	{
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_billboardGBDC, l_billboardPassMeshGPUData, 0, l_billboardPassMeshGPUData.size());
	}

	return true;
}

bool DefaultGPUBuffers::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_MainCameraGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_OpaquePassMeshGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_OpaquePassMaterialGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_SunGBDC);;
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_PointLightGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_SphereLightGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_CSMGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_SkyGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_dispatchParamsGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_GICameraGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_GISkyGBDC);
	g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_billboardGBDC);

	return true;
}

GPUBufferDataComponent * DefaultGPUBuffers::GetGPUBufferDataComponent(GPUBufferUsageType usageType)
{
	GPUBufferDataComponent* l_result;

	switch (usageType)
	{
	case GPUBufferUsageType::MainCamera: l_result = m_MainCameraGBDC;
		break;
	case GPUBufferUsageType::SunShadowPassMesh: l_result = m_SunShadowPassMeshGBDC;
		break;
	case GPUBufferUsageType::OpaquePassMesh: l_result = m_OpaquePassMeshGBDC;
		break;
	case GPUBufferUsageType::OpaquePassMaterial: l_result = m_OpaquePassMaterialGBDC;
		break;
	case GPUBufferUsageType::TransparentPassMesh: l_result = m_TransparentPassMeshGBDC;
		break;
	case GPUBufferUsageType::TransparentPassMaterial: l_result = m_TransparentPassMaterialGBDC;
		break;
	case GPUBufferUsageType::VolumetricFogPassMesh: l_result = m_VolumetricFogPassMeshGBDC;
		break;
	case GPUBufferUsageType::VolumetricFogPassMaterial: l_result = m_VolumetricFogPassMaterialGBDC;
		break;
	case GPUBufferUsageType::Sun: l_result = m_SunGBDC;
		break;
	case GPUBufferUsageType::PointLight: l_result = m_PointLightGBDC;
		break;
	case GPUBufferUsageType::SphereLight: l_result = m_SphereLightGBDC;
		break;
	case GPUBufferUsageType::CSM: l_result = m_CSMGBDC;
		break;
	case GPUBufferUsageType::Sky: l_result = m_SkyGBDC;
		break;
	case GPUBufferUsageType::Compute: l_result = m_dispatchParamsGBDC;
		break;
	case GPUBufferUsageType::GICamera:l_result = m_GICameraGBDC;
		break;
	case GPUBufferUsageType::GISky: l_result = m_GISkyGBDC;
		break;
	case GPUBufferUsageType::Billboard: l_result = m_billboardGBDC;
		break;
	default:
		break;
	}

	return l_result;
}