#include "RenderingContextService.h"

#include "../Common/Timer.h"
#include "../Common/LogService.h"
#include "../Common/TaskScheduler.h"
#include "../Common/DoubleBuffer.h"
#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/ThreadSafeQueue.h"

#include "CullingResult.h"
#include "SceneService.h"
#include "AssetService.h"
#include "GUISystem.h"
#include "TemplateAssetService.h"
#include "RenderingConfigurationService.h"

#include "EntityManager.h"
#include "ComponentManager.h"
#include "PhysicsSimulationService.h"
#include "LightSystem.h"
#include "CameraSystem.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct RenderingContextServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		mutable std::shared_mutex m_Mutex;

		std::vector<AnimationDrawCallInfo> m_animationDrawCallInfoVector;
		std::vector<AnimationConstantBuffer> m_animationCBVector;

		std::vector<TransformConstantBuffer> m_directionalLightPerObjectCB;
		std::vector<TransformConstantBuffer> m_pointLightPerObjectCB;
		std::vector<TransformConstantBuffer> m_sphereLightPerObjectCB;

		std::vector<BillboardPassDrawCallInfo> m_billboardPassDrawCallInfoVector;
		std::vector<TransformConstantBuffer> m_billboardPassPerObjectCB;

		std::vector<DebugPassDrawCallInfo> m_debugPassDrawCallInfoVector;
		std::vector<TransformConstantBuffer> m_debugPassPerObjectCB;

		GPUBufferComponent* m_animationGPUBufferComp;
		GPUBufferComponent* m_billboardGPUBufferComp;

		std::function<void()> f_sceneLoadingFinishedCallback;

		bool Setup(ISystemConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateBillboardPassData();
		bool UpdateDebuggerPassData();
		bool UploadGPUBuffers();
	};
}

bool RenderingContextServiceImpl::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_animationGPUBufferComp = l_renderingServer->AddGPUBufferComponent("AnimationCBuffer/");
	m_billboardGPUBufferComp = l_renderingServer->AddGPUBufferComponent("BillboardCBuffer/");

	f_sceneLoadingFinishedCallback = [&]()
		{
			m_billboardPassDrawCallInfoVector.resize(3);
			m_billboardPassDrawCallInfoVector[0].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
			m_billboardPassDrawCallInfoVector[1].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::POINT_LIGHT);
			m_billboardPassDrawCallInfoVector[2].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::SPHERE_LIGHT);
		};

	// In case no scene is loaded
	f_sceneLoadingFinishedCallback();

	g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RenderingContextServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		auto l_renderingServer = g_Engine->getRenderingServer();

		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_animationGPUBufferComp->m_ElementCount = 512;
		m_animationGPUBufferComp->m_ElementSize = sizeof(AnimationConstantBuffer);

		l_renderingServer->Initialize(m_animationGPUBufferComp);

		m_billboardGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_billboardGPUBufferComp->m_ElementSize = sizeof(TransformConstantBuffer);
		m_billboardGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

		l_renderingServer->Initialize(m_billboardGPUBufferComp);

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "RenderingContextService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "RenderingContextService is not created!");
		return false;
	}
}

bool RenderingContextServiceImpl::UpdateBillboardPassData()
{
	auto& l_lightComponents = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();

	auto l_totalBillboardDrawCallCount = l_lightComponents.size();
	if (l_totalBillboardDrawCallCount == 0)
		return false;

	auto l_billboardPassDrawCallInfoCount = m_billboardPassDrawCallInfoVector.size();
	for (size_t i = 0; i < l_billboardPassDrawCallInfoCount; i++)
	{
		m_billboardPassDrawCallInfoVector[i].instanceCount = 0;
	}

	m_billboardPassPerObjectCB.clear();

	m_directionalLightPerObjectCB.clear();
	m_pointLightPerObjectCB.clear();
	m_sphereLightPerObjectCB.clear();

	for (auto i : l_lightComponents)
	{
		if (i == nullptr)
			continue;

		TransformConstantBuffer l_transformCB;
		l_transformCB.m = Math::toTranslationMatrix(Vec4(i->m_Transform.m_pos, 1.0f));

		switch (i->m_LightType)
		{
		case LightType::Directional:
			m_directionalLightPerObjectCB.emplace_back(l_transformCB);
			m_billboardPassDrawCallInfoVector[0].instanceCount++;
			break;
		case LightType::Point:
			m_pointLightPerObjectCB.emplace_back(l_transformCB);
			m_billboardPassDrawCallInfoVector[1].instanceCount++;
			break;
		case LightType::Spot:
			break;
		case LightType::Sphere:
			m_sphereLightPerObjectCB.emplace_back(l_transformCB);
			m_billboardPassDrawCallInfoVector[2].instanceCount++;
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

	m_billboardPassDrawCallInfoVector[0].meshConstantBufferOffset = 0;
	m_billboardPassDrawCallInfoVector[1].meshConstantBufferOffset = (uint32_t)m_directionalLightPerObjectCB.size();
	m_billboardPassDrawCallInfoVector[2].meshConstantBufferOffset = (uint32_t)(m_directionalLightPerObjectCB.size() + m_pointLightPerObjectCB.size());

	m_billboardPassPerObjectCB.insert(m_billboardPassPerObjectCB.end(), m_directionalLightPerObjectCB.begin(), m_directionalLightPerObjectCB.end());
	m_billboardPassPerObjectCB.insert(m_billboardPassPerObjectCB.end(), m_pointLightPerObjectCB.begin(), m_pointLightPerObjectCB.end());
	m_billboardPassPerObjectCB.insert(m_billboardPassPerObjectCB.end(), m_sphereLightPerObjectCB.begin(), m_sphereLightPerObjectCB.end());

	return true;
}

bool RenderingContextServiceImpl::UpdateDebuggerPassData()
{
	// @TODO: Implementation

	return true;
}

bool RenderingContextServiceImpl::UploadGPUBuffers()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_animationCBVector.size() > 0)
	{
		l_renderingServer->Upload(m_animationGPUBufferComp, m_animationCBVector, 0, m_animationCBVector.size());
	}
	if (m_billboardPassPerObjectCB.size() > 0)
	{
		l_renderingServer->Upload(m_billboardGPUBufferComp, m_billboardPassPerObjectCB, 0, m_billboardPassPerObjectCB.size());
	}

	return true;
}

bool RenderingContextServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		std::lock_guard<std::shared_mutex> l_lock(m_Mutex);

		UpdateBillboardPassData();

		UpdateDebuggerPassData();

		UploadGPUBuffers();

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool RenderingContextServiceImpl::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_animationGPUBufferComp);
	l_renderingServer->Delete(m_billboardGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "RenderingContextService has been terminated.");
	return true;
}

bool RenderingContextService::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new RenderingContextServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool RenderingContextService::Initialize()
{
	return m_Impl->Initialize();
}

bool RenderingContextService::Update()
{
	return m_Impl->Update();
}

bool RenderingContextService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus RenderingContextService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

GPUBufferComponent* RenderingContextService::GetGPUBufferComponent(GPUBufferUsageType usageType)
{
	GPUBufferComponent* l_result;

	switch (usageType)
	{
	case GPUBufferUsageType::Animation: l_result = m_Impl->m_animationGPUBufferComp;
		break;
	case GPUBufferUsageType::Billboard: l_result = m_Impl->m_billboardGPUBufferComp;
		break;
	default:
		break;
	}

	return l_result;
}

const std::vector<AnimationDrawCallInfo>& RenderingContextService::GetAnimationDrawCallInfo()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_animationDrawCallInfoVector;
}

const std::vector<BillboardPassDrawCallInfo>& RenderingContextService::GetBillboardPassDrawCallInfo()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_billboardPassDrawCallInfoVector;
}

const std::vector<DebugPassDrawCallInfo>& RenderingContextService::GetDebugPassDrawCallInfo()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_debugPassDrawCallInfoVector;
}