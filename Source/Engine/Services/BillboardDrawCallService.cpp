#include "BillboardDrawCallService.h"

#include "../Common/LogService.h"
#include "../Common/GPUDataStructure.h"
#include "EntityRegistry.h"
#include "SceneService.h"
#include "TemplateAssetService.h"
#include "RenderingConfigurationService.h"
#include "../Engine.h"

using namespace Inno;

namespace Inno
{
	struct BillboardDrawCallServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		mutable std::shared_mutex m_Mutex;

		std::vector<TransformConstantBuffer> m_DirectionalLightPerObjectCB;
		std::vector<TransformConstantBuffer> m_PointLightPerObjectCB;
		std::vector<TransformConstantBuffer> m_SphereLightPerObjectCB;

		std::vector<BillboardPassDrawCallInfo> m_BillboardPassDrawCallInfoVector;
		std::vector<TransformConstantBuffer> m_BillboardPassPerObjectCB;

		GPUBufferComponent* m_BillboardGPUBufferComp;

		std::function<void()> f_SceneLoadingFinishedCallback;

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateBillboardPassData();
	};
}

bool BillboardDrawCallServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_BillboardGPUBufferComp = l_renderingServer->AddGPUBufferComponent("BillboardCBuffer/");

	f_SceneLoadingFinishedCallback = [&]()
		{
			m_BillboardPassDrawCallInfoVector.resize(3);
			m_BillboardPassDrawCallInfoVector[0].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
			m_BillboardPassDrawCallInfoVector[1].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::POINT_LIGHT);
			m_BillboardPassDrawCallInfoVector[2].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::SPHERE_LIGHT);
		};

	f_SceneLoadingFinishedCallback();

	g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_SceneLoadingFinishedCallback, 0);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool BillboardDrawCallServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		auto l_renderingServer = g_Engine->getRenderingServer();

		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_BillboardGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_BillboardGPUBufferComp->m_ElementSize = sizeof(TransformConstantBuffer);
		m_BillboardGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

		l_renderingServer->Initialize(m_BillboardGPUBufferComp);

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "BillboardDrawCallService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "BillboardDrawCallService is not created!");
		return false;
	}
}

bool BillboardDrawCallServiceImpl::UpdateBillboardPassData()
{
	auto& l_lightComponents = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>().All();

	auto l_totalBillboardDrawCallCount = l_lightComponents.size();
	if (l_totalBillboardDrawCallCount == 0)
		return false;

	auto l_billboardPassDrawCallInfoCount = m_BillboardPassDrawCallInfoVector.size();
	for (size_t i = 0; i < l_billboardPassDrawCallInfoCount; i++)
	{
		m_BillboardPassDrawCallInfoVector[i].instanceCount = 0;
	}

	m_BillboardPassPerObjectCB.clear();

	m_DirectionalLightPerObjectCB.clear();
	m_PointLightPerObjectCB.clear();
	m_SphereLightPerObjectCB.clear();

	for (const auto& i : l_lightComponents)
	{
		TransformConstantBuffer l_transformCB;
		// TODO Phase2-migrate: l_transformCB.m = Math::toTranslationMatrix(Vec4(i.m_Transform.m_pos, 1.0f));
		l_transformCB.m = Mat4();

		switch (i.m_LightType)
		{
		case LightType::Directional:
			m_DirectionalLightPerObjectCB.emplace_back(l_transformCB);
			m_BillboardPassDrawCallInfoVector[0].instanceCount++;
			break;
		case LightType::Point:
			m_PointLightPerObjectCB.emplace_back(l_transformCB);
			m_BillboardPassDrawCallInfoVector[1].instanceCount++;
			break;
		case LightType::Spot:
			break;
		case LightType::Sphere:
			m_SphereLightPerObjectCB.emplace_back(l_transformCB);
			m_BillboardPassDrawCallInfoVector[2].instanceCount++;
			break;
		case LightType::Disk:
			break;
		case LightType::Tube:
			break;
		case LightType::Rectangle:
			break;
		default:
			break;
		}
	}

	m_BillboardPassDrawCallInfoVector[0].meshConstantBufferOffset = 0;
	m_BillboardPassDrawCallInfoVector[1].meshConstantBufferOffset = (uint32_t)m_DirectionalLightPerObjectCB.size();
	m_BillboardPassDrawCallInfoVector[2].meshConstantBufferOffset = (uint32_t)(m_DirectionalLightPerObjectCB.size() + m_PointLightPerObjectCB.size());

	m_BillboardPassPerObjectCB.insert(m_BillboardPassPerObjectCB.end(), m_DirectionalLightPerObjectCB.begin(), m_DirectionalLightPerObjectCB.end());
	m_BillboardPassPerObjectCB.insert(m_BillboardPassPerObjectCB.end(), m_PointLightPerObjectCB.begin(), m_PointLightPerObjectCB.end());
	m_BillboardPassPerObjectCB.insert(m_BillboardPassPerObjectCB.end(), m_SphereLightPerObjectCB.begin(), m_SphereLightPerObjectCB.end());

	return true;
}

bool BillboardDrawCallServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		std::lock_guard<std::shared_mutex> l_lock(m_Mutex);

		UpdateBillboardPassData();

		auto l_renderingServer = g_Engine->getRenderingServer();

		if (m_BillboardPassPerObjectCB.size() > 0)
		{
			l_renderingServer->Upload(m_BillboardGPUBufferComp, m_BillboardPassPerObjectCB, 0, m_BillboardPassPerObjectCB.size());
		}

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool BillboardDrawCallServiceImpl::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_BillboardGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "BillboardDrawCallService has been terminated.");
	return true;
}

bool BillboardDrawCallService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new BillboardDrawCallServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool BillboardDrawCallService::Initialize()
{
	return m_Impl->Initialize();
}

bool BillboardDrawCallService::Update()
{
	return m_Impl->Update();
}

bool BillboardDrawCallService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus BillboardDrawCallService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

const std::vector<BillboardPassDrawCallInfo>& BillboardDrawCallService::GetBillboardPassDrawCallInfo()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_BillboardPassDrawCallInfoVector;
}

GPUBufferComponent* BillboardDrawCallService::GetBillboardBuffer()
{
	return m_Impl->m_BillboardGPUBufferComp;
}
