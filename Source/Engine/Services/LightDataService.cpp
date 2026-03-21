#include "LightDataService.h"

#include "../Common/LogService.h"
#include "../Common/GPUDataStructure.h"
#include "ComponentManager.h"
#include "RenderingConfigurationService.h"
#include "../Component/LightComponent.h"
#include "../Engine.h"

using namespace Inno;

namespace Inno
{
	struct LightDataServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		std::vector<PointLightConstantBuffer> m_PointLightCBVector;
		std::vector<SphereLightConstantBuffer> m_SphereLightCBVector;
		std::vector<CSMConstantBuffer> m_CSMCBVector;

		GPUBufferComponent* m_PointLightGPUBufferComp;
		GPUBufferComponent* m_SphereLightGPUBufferComp;
		GPUBufferComponent* m_CSMGPUBufferComp;
		GPUBufferComponent* m_GICBufferGPUBufferComp;

		bool Setup(ISystemConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateLightData();
		bool UpdateCSMData();
	};
}

bool LightDataServiceImpl::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_PointLightGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PointLightCBuffer/");
	m_SphereLightGPUBufferComp = l_renderingServer->AddGPUBufferComponent("SphereLightCBuffer/");
	m_CSMGPUBufferComp = l_renderingServer->AddGPUBufferComponent("CSMCBuffer/");
	m_GICBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("GICBuffer/");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool LightDataServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		auto l_renderingServer = g_Engine->getRenderingServer();
		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_PointLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxPointLights;
		m_PointLightGPUBufferComp->m_ElementSize = sizeof(PointLightConstantBuffer);

		l_renderingServer->Initialize(m_PointLightGPUBufferComp);

		m_SphereLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxSphereLights;
		m_SphereLightGPUBufferComp->m_ElementSize = sizeof(SphereLightConstantBuffer);

		l_renderingServer->Initialize(m_SphereLightGPUBufferComp);

		m_CSMGPUBufferComp->m_ElementCount = l_RenderingCapability.maxCSMSplits;
		m_CSMGPUBufferComp->m_ElementSize = sizeof(CSMConstantBuffer);

		l_renderingServer->Initialize(m_CSMGPUBufferComp);

		m_GICBufferGPUBufferComp->m_ElementSize = sizeof(GIConstantBuffer);
		m_GICBufferGPUBufferComp->m_ElementCount = 1;

		l_renderingServer->Initialize(m_GICBufferGPUBufferComp);

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "LightDataService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "LightDataService is not created!");
		return false;
	}
}

bool LightDataServiceImpl::UpdateLightData()
{
	m_PointLightCBVector.clear();
	m_SphereLightCBVector.clear();

	auto& l_lightComponents = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();
	auto l_lightComponentCount = l_lightComponents.size();

	if (l_lightComponentCount == 0)
		return false;

	for (size_t i = 0; i < l_lightComponentCount; i++)
	{
		auto l_lightComponent = l_lightComponents[i];
		if (l_lightComponent == nullptr)
			continue;

		if (l_lightComponent->m_LightType == LightType::Point)
		{
			PointLightConstantBuffer l_data;
			// TODO Phase2-migrate: l_data.pos = l_lightComponent->m_Transform.m_pos;
			l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
			l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
			m_PointLightCBVector.emplace_back(l_data);
		}
		else if (l_lightComponents[i]->m_LightType == LightType::Sphere)
		{
			SphereLightConstantBuffer l_data;
			// TODO Phase2-migrate: l_data.pos = l_lightComponent->m_Transform.m_pos;
			l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
			l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
			m_SphereLightCBVector.emplace_back(l_data);
		}
	}

	return true;
}

bool LightDataServiceImpl::UpdateCSMData()
{
	auto l_sun = g_Engine->Get<ComponentManager>()->Get<LightComponent>(0);
	if (l_sun == nullptr)
		return false;

	auto& l_LitRegion_WorldSpace = l_sun->m_LitRegion_WorldSpace;
	auto& l_ViewMatrices = l_sun->m_ViewMatrices;
	auto& l_ProjectionMatrices = l_sun->m_ProjectionMatrices;

	m_CSMCBVector.clear();

	if (l_LitRegion_WorldSpace.size() > 0 && l_ViewMatrices.size() > 0 && l_ProjectionMatrices.size() > 0)
	{
		for (size_t j = 0; j < l_LitRegion_WorldSpace.size(); j++)
		{
			CSMConstantBuffer l_CSMCB;

			l_CSMCB.p = l_ProjectionMatrices[j];
			l_CSMCB.v = l_ViewMatrices[j];

			l_CSMCB.AABBMax = l_LitRegion_WorldSpace[j].m_boundMax;
			l_CSMCB.AABBMin = l_LitRegion_WorldSpace[j].m_boundMin;

			m_CSMCBVector.emplace_back(l_CSMCB);
		}
	}

	return true;
}

bool LightDataServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		UpdateLightData();
		UpdateCSMData();

		auto l_renderingServer = g_Engine->getRenderingServer();

		if (m_PointLightCBVector.size() > 0)
		{
			l_renderingServer->Upload(m_PointLightGPUBufferComp, m_PointLightCBVector, 0, m_PointLightCBVector.size());
		}
		if (m_SphereLightCBVector.size() > 0)
		{
			l_renderingServer->Upload(m_SphereLightGPUBufferComp, m_SphereLightCBVector, 0, m_SphereLightCBVector.size());
		}
		if (m_CSMCBVector.size() > 0)
		{
			l_renderingServer->Upload(m_CSMGPUBufferComp, m_CSMCBVector, 0, m_CSMCBVector.size());
		}

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool LightDataServiceImpl::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_PointLightGPUBufferComp);
	l_renderingServer->Delete(m_SphereLightGPUBufferComp);
	l_renderingServer->Delete(m_CSMGPUBufferComp);
	l_renderingServer->Delete(m_GICBufferGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "LightDataService has been terminated.");
	return true;
}

bool LightDataService::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new LightDataServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool LightDataService::Initialize()
{
	return m_Impl->Initialize();
}

bool LightDataService::Update()
{
	return m_Impl->Update();
}

bool LightDataService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus LightDataService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

GPUBufferComponent* LightDataService::GetPointLightBuffer()
{
	return m_Impl->m_PointLightGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetSphereLightBuffer()
{
	return m_Impl->m_SphereLightGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetCSMBuffer()
{
	return m_Impl->m_CSMGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetGIBuffer()
{
	return m_Impl->m_GICBufferGPUBufferComp;
}
