#include "LightDataService.h"

#include "../Common/LogService.h"
#include "../Common/Array.h"
#include "../Common/GPUDataStructure.h"
#include "EntityRegistry.h"
#include "RenderingConfigurationService.h"
#include "../Component/LightComponent.h"
#include "../Component/TransformComponent.h"
#include "../Engine.h"
#include "GPUBufferResourceService.h"

using namespace Inno;

namespace Inno
{
	struct LightDataServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		Inno::Array<PointLightConstantBuffer>  m_PointLightCBVector;
		Inno::Array<SphereLightConstantBuffer> m_SphereLightCBVector;

		GPUBufferComponent* m_PointLightGPUBufferComp   = nullptr;
		GPUBufferComponent* m_SphereLightGPUBufferComp  = nullptr;
		GPUBufferComponent* m_GICBufferGPUBufferComp    = nullptr;

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateLightData();
	};
}

bool LightDataServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

	m_PointLightGPUBufferComp = l_rsService->Add("PointLightCBuffer");
	m_SphereLightGPUBufferComp = l_rsService->Add("SphereLightCBuffer");
	m_GICBufferGPUBufferComp = l_rsService->Add("GICBuffer");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool LightDataServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();
		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_PointLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxPointLights;
		m_PointLightGPUBufferComp->m_ElementSize = sizeof(PointLightConstantBuffer);

		l_rsService->Initialize(m_PointLightGPUBufferComp);

		m_SphereLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxSphereLights;
		m_SphereLightGPUBufferComp->m_ElementSize = sizeof(SphereLightConstantBuffer);

		l_rsService->Initialize(m_SphereLightGPUBufferComp);

		m_GICBufferGPUBufferComp->m_ElementSize = sizeof(GIConstantBuffer);
		m_GICBufferGPUBufferComp->m_ElementCount = 1;

		l_rsService->Initialize(m_GICBufferGPUBufferComp);

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

	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	const auto& l_Lights = l_Storage.All();
	const auto& l_Owners = l_Storage.AllOwners();

	if (l_Lights.empty())
		return false;

	for (size_t i = 0; i < l_Lights.size(); i++)
	{
		const LightComponent& l_Light = l_Lights[i];
		EntityID l_EntityID = l_Owners[i];
		auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_EntityID);

		if (l_Light.m_LightType == LightType::Point)
		{
			PointLightConstantBuffer l_data;
			if (l_Transform)
				l_data.pos = l_Transform->m_LocalPos;
			l_data.luminance = l_Light.m_RGBColor * l_Light.m_LuminousFlux;
			l_data.luminance.w = l_Light.m_Shape.x;
			l_data.m_CastShadow = l_Light.m_CastShadow ? 1u : 0u;
			m_PointLightCBVector.emplace_back(l_data);
		}
		else if (l_Light.m_LightType == LightType::Sphere)
		{
			SphereLightConstantBuffer l_data;
			if (l_Transform)
				l_data.pos = l_Transform->m_LocalPos;
			l_data.luminance = l_Light.m_RGBColor * l_Light.m_LuminousFlux;
			l_data.luminance.w = l_Light.m_Shape.x;
			m_SphereLightCBVector.emplace_back(l_data);
		}
	}

	return true;
}

bool LightDataServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		UpdateLightData();

	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

		if (m_PointLightCBVector.size() > 0)
		{
			l_rsService->Upload(m_PointLightGPUBufferComp, m_PointLightCBVector, 0, m_PointLightCBVector.size());
		}
		if (m_SphereLightCBVector.size() > 0)
		{
			l_rsService->Upload(m_SphereLightGPUBufferComp, m_SphereLightCBVector, 0, m_SphereLightCBVector.size());
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
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

	l_rsService->Delete(m_PointLightGPUBufferComp);
	l_rsService->Delete(m_SphereLightGPUBufferComp);
	l_rsService->Delete(m_GICBufferGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "LightDataService has been terminated.");
	return true;
}

bool LightDataService::Setup(IServiceConfig* systemConfig)
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

GPUBufferComponent* LightDataService::GetGIBuffer()
{
	return m_Impl->m_GICBufferGPUBufferComp;
}

uint32_t LightDataService::GetPointLightCount()
{
	return static_cast<uint32_t>(m_Impl->m_PointLightCBVector.size());
}

uint32_t LightDataService::GetSphereLightCount()
{
	return static_cast<uint32_t>(m_Impl->m_SphereLightCBVector.size());
}
