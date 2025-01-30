#include "RenderingContextService.h"

#include "../Common/Timer.h"
#include "../Common/LogService.h"
#include "../Common/TaskScheduler.h"
#include "../Common/DoubleBuffer.h"
#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/ThreadSafeQueue.h"

#include "CullingResult.h"
#include "SceneSystem.h"
#include "AssetSystem.h"
#include "GUISystem.h"
#include "TemplateAssetService.h"
#include "RenderingConfigurationService.h"

#include "EntityManager.h"
#include "ComponentManager.h"
#include "PhysicsSystem.h"
#include "TransformSystem.h"
#include "LightSystem.h"
#include "CameraSystem.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct RenderingContextServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		DoubleBuffer<PerFrameConstantBuffer, true> m_perFrameCB;

		DoubleBuffer<std::vector<CSMConstantBuffer>, true> m_CSMCBVector;

		DoubleBuffer<std::vector<PointLightConstantBuffer>, true> m_pointLightCBVector;
		DoubleBuffer<std::vector<SphereLightConstantBuffer>, true> m_sphereLightCBVector;

		uint32_t m_drawCallCount = 0;
		DoubleBuffer<std::vector<DrawCallInfo>, true> m_drawCallInfoVector;
		DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_perObjectCBVector;
		DoubleBuffer<std::vector<MaterialConstantBuffer>, true> m_materialCBVector;

		DoubleBuffer<std::vector<AnimationDrawCallInfo>, true> m_animationDrawCallInfoVector;
		DoubleBuffer<std::vector<AnimationConstantBuffer>, true> m_animationCBVector;

		DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_directionalLightPerObjectCB;
		DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_pointLightPerObjectCB;
		DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_sphereLightPerObjectCB;

		DoubleBuffer<std::vector<BillboardPassDrawCallInfo>, true> m_billboardPassDrawCallInfoVector;
		DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_billboardPassPerObjectCB;

		DoubleBuffer<std::vector<DebugPassDrawCallInfo>, true> m_debugPassDrawCallInfoVector;
		DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_debugPassPerObjectCB;

		std::vector<CullingResult> m_cullingResults;

		std::vector<Vec2> m_haltonSampler;
		int32_t m_currentHaltonStep = 0;

		std::function<void()> f_sceneLoadingStartedCallback;
		std::function<void()> f_sceneLoadingFinishedCallback;

		bool Setup(ISystemConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		float radicalInverse(uint32_t n, uint32_t base);
		void initializeHaltonSampler();

		bool updatePerFrameConstantBuffer();
		bool updateLightData();
		bool updateMeshData();
		bool updateBillboardPassData();
		bool updateDebuggerPassData();
	};
}

float RenderingContextServiceImpl::radicalInverse(uint32_t n, uint32_t base)
{
	float val = 0.0f;
	float invBase = 1.0f / base, invBi = invBase;
	while (n > 0)
	{
		auto d_i = (n % base);
		val += d_i * invBi;
		n *= (uint32_t)invBase;
		invBi *= invBase;
	}
	return val;
};

void RenderingContextServiceImpl::initializeHaltonSampler()
{
	// in NDC space
	for (uint32_t i = 0; i < 16; i++)
	{
		m_haltonSampler.emplace_back(Vec2(radicalInverse(i, 3) * 2.0f - 1.0f, radicalInverse(i, 4) * 2.0f - 1.0f));
	}
}

bool RenderingContextServiceImpl::Setup(ISystemConfig* systemConfig)
{
	f_sceneLoadingStartedCallback = [&]()
		{
			Log(Verbose, "Clearing all rendering context data...");

			m_cullingResults.clear();

			m_drawCallCount = 0;

			Log(Success, "All rendering context data has been cleared.");
		};

	f_sceneLoadingFinishedCallback = [&]()
		{
			// @TODO:
			std::vector<BillboardPassDrawCallInfo> l_billboardPassDrawCallInfoVectorA(3);
			l_billboardPassDrawCallInfoVectorA[0].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
			l_billboardPassDrawCallInfoVectorA[1].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::POINT_LIGHT);
			l_billboardPassDrawCallInfoVectorA[2].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::SPHERE_LIGHT);
			auto l_billboardPassDrawCallInfoVectorB = l_billboardPassDrawCallInfoVectorA;

			m_billboardPassDrawCallInfoVector.SetValue(std::move(l_billboardPassDrawCallInfoVectorA));
			m_billboardPassDrawCallInfoVector.SetValue(std::move(l_billboardPassDrawCallInfoVectorB));
		};

	g_Engine->Get<SceneSystem>()->AddSceneLoadingStartedCallback(&f_sceneLoadingStartedCallback, 0);
	g_Engine->Get<SceneSystem>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RenderingContextServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		initializeHaltonSampler();

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

bool RenderingContextServiceImpl::updatePerFrameConstantBuffer()
{
	auto l_camera = static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->GetActiveCamera();
	if (l_camera == nullptr)
		return false;

	auto l_cameraTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_camera->m_Owner);
	if (l_cameraTransformComponent == nullptr)
		return false;

	PerFrameConstantBuffer l_PerFrameCB;

	auto l_p = l_camera->m_projectionMatrix;

	l_PerFrameCB.p_original = l_p;
	l_PerFrameCB.p_jittered = l_p;

	auto l_renderingConfigurationService = g_Engine->Get<RenderingConfigurationService>();
	auto l_renderingConfig = l_renderingConfigurationService->GetRenderingConfig();
	auto l_screenResolution = l_renderingConfigurationService->GetScreenResolution();
	if (l_renderingConfig.useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = m_currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		l_PerFrameCB.p_jittered.m02 = m_haltonSampler[l_currentHaltonStep].x / l_screenResolution.x;
		l_PerFrameCB.p_jittered.m12 = m_haltonSampler[l_currentHaltonStep].y / l_screenResolution.y;
		l_currentHaltonStep += 1;
	}

	auto r = Math::getInvertRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);

	auto t = Math::getInvertTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);

	l_PerFrameCB.camera_posWS = l_cameraTransformComponent->m_globalTransformVector.m_pos;

	l_PerFrameCB.v = r * t;

	auto r_prev = l_cameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	auto t_prev = l_cameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	l_PerFrameCB.v_prev = r_prev * t_prev;

	l_PerFrameCB.zNear = l_camera->m_zNear;
	l_PerFrameCB.zFar = l_camera->m_zFar;

	l_PerFrameCB.p_inv = l_p.inverse();
	l_PerFrameCB.v_inv = l_PerFrameCB.v.inverse();
	l_PerFrameCB.viewportSize.x = (float)l_screenResolution.x;
	l_PerFrameCB.viewportSize.y = (float)l_screenResolution.y;
	l_PerFrameCB.minLogLuminance = -10.0f;
	l_PerFrameCB.maxLogLuminance = 16.0f;
	l_PerFrameCB.aperture = l_camera->m_aperture;
	l_PerFrameCB.shutterTime = l_camera->m_shutterTime;
	l_PerFrameCB.ISO = l_camera->m_ISO;

	auto l_sun = g_Engine->Get<ComponentManager>()->Get<LightComponent>(0);
	if (l_sun == nullptr)
		return false;

	auto l_sunTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_sun->m_Owner);
	if (l_sunTransformComponent == nullptr)
		return false;

	l_PerFrameCB.sun_direction = Math::getDirection(Direction::Backward, l_sunTransformComponent->m_globalTransformVector.m_rot);
	l_PerFrameCB.sun_illuminance = l_sun->m_RGBColor * l_sun->m_LuminousFlux;

	static uint32_t currentCascade = 0;
	auto l_renderingCapability = l_renderingConfigurationService->GetRenderingCapability();
	currentCascade = currentCascade < l_renderingCapability.maxCSMSplits - 1 ? ++currentCascade : 0;
	l_PerFrameCB.activeCascade = currentCascade;
	m_perFrameCB.SetValue(std::move(l_PerFrameCB));

	auto& l_SplitAABB = l_sun->m_SplitAABBWS;
	auto& l_ViewMatrices = l_sun->m_ViewMatrices;
	auto& l_ProjectionMatrices = l_sun->m_ProjectionMatrices;

	auto& l_CSMCBVector = m_CSMCBVector.GetOldValue();
	l_CSMCBVector.clear();

	if (l_SplitAABB.size() > 0 && l_ViewMatrices.size() > 0 && l_ProjectionMatrices.size() > 0)
	{
		for (size_t j = 0; j < l_SplitAABB.size(); j++)
		{
			CSMConstantBuffer l_CSMCB;

			l_CSMCB.p = l_ProjectionMatrices[j];
			l_CSMCB.v = l_ViewMatrices[j];

			l_CSMCB.AABBMax = l_SplitAABB[j].m_boundMax;
			l_CSMCB.AABBMin = l_SplitAABB[j].m_boundMin;

			l_CSMCBVector.emplace_back(l_CSMCB);
		}
	}

	m_CSMCBVector.SetValue(std::move(l_CSMCBVector));

	return true;
}

bool RenderingContextServiceImpl::updateLightData()
{
	auto& l_PointLightCB = m_pointLightCBVector.GetOldValue();
	auto& l_SphereLightCB = m_sphereLightCBVector.GetOldValue();

	l_PointLightCB.clear();
	l_SphereLightCB.clear();

	auto& l_lightComponents = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();
	auto l_lightComponentCount = l_lightComponents.size();

	if (l_lightComponentCount > 0)
	{
		for (size_t i = 0; i < l_lightComponentCount; i++)
		{
			auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_lightComponents[i]->m_Owner);
			if (l_transformComponent != nullptr)
			{
				if (l_lightComponents[i]->m_LightType == LightType::Point)
				{
					PointLightConstantBuffer l_data;
					l_data.pos = l_transformComponent->m_globalTransformVector.m_pos;
					l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
					l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
					l_PointLightCB.emplace_back(l_data);
				}
				else if (l_lightComponents[i]->m_LightType == LightType::Sphere)
				{
					SphereLightConstantBuffer l_data;
					l_data.pos = l_transformComponent->m_globalTransformVector.m_pos;
					l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
					l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
					l_SphereLightCB.emplace_back(l_data);
				}
			}
		}
	}

	m_pointLightCBVector.SetValue(std::move(l_PointLightCB));
	m_sphereLightCBVector.SetValue(std::move(l_SphereLightCB));

	return true;
}

bool RenderingContextServiceImpl::updateMeshData()
{
	auto& l_drawCallInfoVector = m_drawCallInfoVector.GetOldValue();
	auto& l_perObjectCBVector = m_perObjectCBVector.GetOldValue();
	auto& l_materialCBVector = m_materialCBVector.GetOldValue();
	auto& l_animationDrawCallInfoVector = m_animationDrawCallInfoVector.GetOldValue();
	auto& l_animationCBVector = m_animationCBVector.GetOldValue();

	l_drawCallInfoVector.clear();
	l_perObjectCBVector.clear();
	l_materialCBVector.clear();
	l_animationDrawCallInfoVector.clear();
	l_animationCBVector.clear();

	uint32_t l_drawCallIndex = 0;
	auto l_cullingResultCount = m_cullingResults.size();
	for (size_t i = 0; i < l_cullingResultCount; i++)
	{
		auto l_cullingResult = m_cullingResults[i];
		if (l_cullingResult.m_PhysicsComponent == nullptr)
			continue;

		auto l_modelComponent = l_cullingResult.m_PhysicsComponent->m_ModelComponent;
		if (l_modelComponent == nullptr)
			continue;

		if (l_modelComponent->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_modelComponent->m_Owner);
		if (l_transformComponent == nullptr)
			continue;

		if (l_transformComponent->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		auto l_renderableSet = l_cullingResult.m_PhysicsComponent->m_RenderableSet;
		if (l_renderableSet == nullptr)
			continue;

		auto l_mesh = l_renderableSet->mesh;
		if (l_mesh == nullptr)
			continue;

		if (l_mesh->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		auto l_material = l_renderableSet->material;
		if (l_material == nullptr)
			continue;

		if (l_material->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		DrawCallInfo l_drawCallInfo;

		l_drawCallInfo.mesh = l_mesh;
		l_drawCallInfo.m_PerObjectConstantBufferIndex = l_drawCallIndex;
		l_drawCallInfo.meshUsage = l_modelComponent->m_meshUsage;
		l_drawCallInfo.m_VisibilityMask = l_cullingResult.m_VisibilityMask;

		PerObjectConstantBuffer l_perObjectCB;
		l_perObjectCB.m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
		l_perObjectCB.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
		l_perObjectCB.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
		l_perObjectCB.UUID = (float)l_transformComponent->m_UUID;
		l_perObjectCB.m_MaterialIndex = l_drawCallIndex; // @TODO: The material is duplicated per object, this should be fixed

		l_perObjectCBVector.emplace_back(l_perObjectCB);

		MaterialConstantBuffer l_materialCB;
		l_materialCB.m_MaterialAttributes = l_material->m_materialAttributes;

		auto l_renderingServer = g_Engine->getRenderingServer();
		for (size_t i = 0; i < MaxTextureSlotCount; i++)
		{
			l_materialCB.m_TextureIndices[i] = l_renderingServer->GetIndex(l_material->m_TextureSlots[i].m_Texture, Accessibility::ReadOnly);
		}

		l_materialCBVector.emplace_back(l_materialCB);
		l_drawCallInfoVector.emplace_back(l_drawCallInfo);
		l_drawCallIndex++;

		if (l_modelComponent->m_meshUsage == MeshUsage::Skeletal)
		{
			auto l_result = g_Engine->Get<AnimationService>()->GetAnimationInstance(l_modelComponent->m_UUID);
			if (l_result.animationData.ADC == nullptr)
				continue;

			AnimationDrawCallInfo animationDrawCallInfo;
			animationDrawCallInfo.animationInstance = l_result;
			animationDrawCallInfo.drawCallInfo = l_drawCallInfo;

			AnimationConstantBuffer l_animationCB;
			l_animationCB.duration = animationDrawCallInfo.animationInstance.animationData.ADC->m_Duration;
			l_animationCB.numChannels = animationDrawCallInfo.animationInstance.animationData.ADC->m_NumChannels;
			l_animationCB.numTicks = animationDrawCallInfo.animationInstance.animationData.ADC->m_NumTicks;
			l_animationCB.currentTime = animationDrawCallInfo.animationInstance.currentTime / l_animationCB.duration;
			l_animationCB.rootOffsetMatrix = Math::generateIdentityMatrix<float>();

			l_animationCBVector.emplace_back(l_animationCB);

			animationDrawCallInfo.animationConstantBufferIndex = (uint32_t)l_animationCBVector.size();
			l_animationDrawCallInfoVector.emplace_back(animationDrawCallInfo);
		}
	}

	m_drawCallInfoVector.SetValue(std::move(l_drawCallInfoVector));
	m_perObjectCBVector.SetValue(std::move(l_perObjectCBVector));
	m_materialCBVector.SetValue(std::move(l_materialCBVector));
	m_animationDrawCallInfoVector.SetValue(std::move(l_animationDrawCallInfoVector));
	m_animationCBVector.SetValue(std::move(l_animationCBVector));

	// @TODO: use GPU to do OIT

	return true;
}

bool RenderingContextServiceImpl::updateBillboardPassData()
{
	auto& l_lightComponents = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();

	auto l_totalBillboardDrawCallCount = l_lightComponents.size();

	if (l_totalBillboardDrawCallCount == 0)
	{
		return false;
	}

	auto& l_billboardPassDrawCallInfoVector = m_billboardPassDrawCallInfoVector.GetOldValue();
	auto& l_billboardPassPerObjectCB = m_billboardPassPerObjectCB.GetOldValue();
	auto& l_directionalLightPerObjectCB = m_directionalLightPerObjectCB.GetOldValue();
	auto& l_pointLightPerObjectCB = m_pointLightPerObjectCB.GetOldValue();
	auto& l_sphereLightPerObjectCB = m_sphereLightPerObjectCB.GetOldValue();

	auto l_billboardPassDrawCallInfoCount = l_billboardPassDrawCallInfoVector.size();
	for (size_t i = 0; i < l_billboardPassDrawCallInfoCount; i++)
	{
		l_billboardPassDrawCallInfoVector[i].instanceCount = 0;
	}

	l_billboardPassPerObjectCB.clear();

	l_directionalLightPerObjectCB.clear();
	l_pointLightPerObjectCB.clear();
	l_sphereLightPerObjectCB.clear();

	for (auto i : l_lightComponents)
	{
		PerObjectConstantBuffer l_meshCB;

		auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(i->m_Owner);
		if (l_transformComponent != nullptr)
		{
			l_meshCB.m = Math::toTranslationMatrix(l_transformComponent->m_globalTransformVector.m_pos);
		}

		switch (i->m_LightType)
		{
		case LightType::Directional:
			l_directionalLightPerObjectCB.emplace_back(l_meshCB);
			l_billboardPassDrawCallInfoVector[0].instanceCount++;
			break;
		case LightType::Point:
			l_pointLightPerObjectCB.emplace_back(l_meshCB);
			l_billboardPassDrawCallInfoVector[1].instanceCount++;
			break;
		case LightType::Spot:
			break;
		case LightType::Sphere:
			l_sphereLightPerObjectCB.emplace_back(l_meshCB);
			l_billboardPassDrawCallInfoVector[2].instanceCount++;
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

	l_billboardPassDrawCallInfoVector[0].meshConstantBufferOffset = 0;
	l_billboardPassDrawCallInfoVector[1].meshConstantBufferOffset = (uint32_t)l_directionalLightPerObjectCB.size();
	l_billboardPassDrawCallInfoVector[2].meshConstantBufferOffset = (uint32_t)(l_directionalLightPerObjectCB.size() + l_pointLightPerObjectCB.size());

	l_billboardPassPerObjectCB.insert(l_billboardPassPerObjectCB.end(), l_directionalLightPerObjectCB.begin(), l_directionalLightPerObjectCB.end());
	l_billboardPassPerObjectCB.insert(l_billboardPassPerObjectCB.end(), l_pointLightPerObjectCB.begin(), l_pointLightPerObjectCB.end());
	l_billboardPassPerObjectCB.insert(l_billboardPassPerObjectCB.end(), l_sphereLightPerObjectCB.begin(), l_sphereLightPerObjectCB.end());

	m_billboardPassDrawCallInfoVector.SetValue(std::move(l_billboardPassDrawCallInfoVector));
	m_billboardPassPerObjectCB.SetValue(std::move(l_billboardPassPerObjectCB));
	m_directionalLightPerObjectCB.SetValue(std::move(l_directionalLightPerObjectCB));
	m_pointLightPerObjectCB.SetValue(std::move(l_pointLightPerObjectCB));
	m_sphereLightPerObjectCB.SetValue(std::move(l_sphereLightPerObjectCB));

	return true;
}

bool RenderingContextServiceImpl::updateDebuggerPassData()
{
	// @TODO: Implementation

	return true;
}

bool RenderingContextServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		updatePerFrameConstantBuffer();

		updateLightData();

		// copy culling data pack for local scope
		m_cullingResults = g_Engine->Get<PhysicsSystem>()->GetCullingResult();

		updateMeshData();

		updateBillboardPassData();

		updateDebuggerPassData();

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
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "RenderingContextService has been terminated.");
	return true;
}

bool RenderingContextService::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new RenderingContextServiceImpl();

	g_Engine->Get<ComponentManager>()->RegisterType<SkeletonComponent>(2048, this);
	g_Engine->Get<ComponentManager>()->RegisterType<AnimationComponent>(16384, this);

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

const PerFrameConstantBuffer& RenderingContextService::GetPerFrameConstantBuffer()
{
	return m_Impl->m_perFrameCB.GetNewValue();
}

const std::vector<CSMConstantBuffer>& RenderingContextService::GetCSMConstantBuffer()
{
	return m_Impl->m_CSMCBVector.GetNewValue();
}

const std::vector<PointLightConstantBuffer>& RenderingContextService::GetPointLightConstantBuffer()
{
	return m_Impl->m_pointLightCBVector.GetNewValue();
}

const std::vector<SphereLightConstantBuffer>& RenderingContextService::GetSphereLightConstantBuffer()
{
	return m_Impl->m_sphereLightCBVector.GetNewValue();
}

const std::vector<DrawCallInfo>& RenderingContextService::GetDrawCallInfo()
{
	return m_Impl->m_drawCallInfoVector.GetNewValue();
}

const std::vector<PerObjectConstantBuffer>& RenderingContextService::GetPerObjectConstantBuffer()
{
	return m_Impl->m_perObjectCBVector.GetNewValue();
}

const std::vector<MaterialConstantBuffer>& RenderingContextService::GetMaterialConstantBuffer()
{
	return m_Impl->m_materialCBVector.GetNewValue();
}

const std::vector<AnimationDrawCallInfo>& RenderingContextService::GetAnimationDrawCallInfo()
{
	return m_Impl->m_animationDrawCallInfoVector.GetNewValue();
}

const std::vector<AnimationConstantBuffer>& RenderingContextService::GetAnimationConstantBuffer()
{
	return m_Impl->m_animationCBVector.GetNewValue();
}

const std::vector<BillboardPassDrawCallInfo>& RenderingContextService::GetBillboardPassDrawCallInfo()
{
	return m_Impl->m_billboardPassDrawCallInfoVector.GetNewValue();
}

const std::vector<PerObjectConstantBuffer>& RenderingContextService::GetBillboardPassPerObjectConstantBuffer()
{
	return m_Impl->m_billboardPassPerObjectCB.GetNewValue();
}

const std::vector<DebugPassDrawCallInfo>& RenderingContextService::GetDebugPassDrawCallInfo()
{
	return m_Impl->m_debugPassDrawCallInfoVector.GetNewValue();
}

const std::vector<PerObjectConstantBuffer>& RenderingContextService::GetDebugPassPerObjectConstantBuffer()
{
	return m_Impl->m_debugPassPerObjectCB.GetNewValue();
}