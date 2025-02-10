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

		mutable std::shared_mutex m_Mutex;

		std::vector<PerFrameConstantBuffer> m_perFrameCBs;

		std::vector<CSMConstantBuffer> m_CSMCBVector;

		std::vector<PointLightConstantBuffer> m_pointLightCBVector;
		std::vector<SphereLightConstantBuffer> m_sphereLightCBVector;

		std::vector<DrawCallInfo> m_drawCallInfoVector;
		std::vector<PerObjectConstantBuffer> m_perObjectCBVector;
		std::vector<MaterialConstantBuffer> m_materialCBVector;

		std::vector<AnimationDrawCallInfo> m_animationDrawCallInfoVector;
		std::vector<AnimationConstantBuffer> m_animationCBVector;

		std::vector<PerObjectConstantBuffer> m_directionalLightPerObjectCB;
		std::vector<PerObjectConstantBuffer> m_pointLightPerObjectCB;
		std::vector<PerObjectConstantBuffer> m_sphereLightPerObjectCB;

		std::vector<BillboardPassDrawCallInfo> m_billboardPassDrawCallInfoVector;
		std::vector<PerObjectConstantBuffer> m_billboardPassPerObjectCB;

		std::vector<DebugPassDrawCallInfo> m_debugPassDrawCallInfoVector;
		std::vector<PerObjectConstantBuffer> m_debugPassPerObjectCB;

		GPUBufferComponent* m_PerFrameCBufferGPUBufferComp;
		GPUBufferComponent* m_PerObjectGPUBufferComp;
		GPUBufferComponent* m_MaterialGPUBufferComp;
		GPUBufferComponent* m_PointLightGPUBufferComp;
		GPUBufferComponent* m_SphereLightGPUBufferComp;
		GPUBufferComponent* m_CSMGPUBufferComp;
		GPUBufferComponent* m_DispatchParamsGPUBufferComp;
		GPUBufferComponent* m_GICBufferGPUBufferComp;
		GPUBufferComponent* m_animationGPUBufferComp;
		GPUBufferComponent* m_billboardGPUBufferComp;

		std::vector<DispatchParamsConstantBuffer> m_DispatchParamsConstantBuffer;

		struct HaltonSampler
		{
			std::vector<Vec2> m_Values;
			int32_t m_CurrentStep = 0;
		};

		std::vector<HaltonSampler> m_HaltonSamplers;

		std::function<void()> f_sceneLoadingFinishedCallback;

		bool Setup(ISystemConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		float RadicalInverse(uint32_t n, uint32_t base);
		void InitializeHaltonSampler();

		bool UpdatePerFrameConstantBuffer();
		bool UpdateLightData();
		bool UpdateDrawCalls();
		bool UpdateBillboardPassData();
		bool UpdateDebuggerPassData();
		bool UploadGPUBuffers();
	};
}

float RenderingContextServiceImpl::RadicalInverse(uint32_t n, uint32_t base)
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

void RenderingContextServiceImpl::InitializeHaltonSampler()
{
	// in NDC space
	auto l_swapChainImageCount = g_Engine->getRenderingServer()->GetSwapChainImageCount();
	m_HaltonSamplers.resize(l_swapChainImageCount);
	for (auto& m_haltonSampler : m_HaltonSamplers)
	{
		m_haltonSampler.m_Values.reserve(16);
		for (uint32_t i = 0; i < 16; i++)
		{
			m_haltonSampler.m_Values.emplace_back(Vec2(RadicalInverse(i, 3) * 2.0f - 1.0f, RadicalInverse(i, 4) * 2.0f - 1.0f));
		}		
	}
}

bool RenderingContextServiceImpl::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_PerFrameCBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PerFrameCBuffer/");
	m_PerObjectGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PerObjectCBuffer/");
	m_MaterialGPUBufferComp = l_renderingServer->AddGPUBufferComponent("MaterialCBuffer/");
	m_PointLightGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PointLightCBuffer/");
	m_SphereLightGPUBufferComp = l_renderingServer->AddGPUBufferComponent("SphereLightCBuffer/");
	m_CSMGPUBufferComp = l_renderingServer->AddGPUBufferComponent("CSMCBuffer/");
	m_DispatchParamsGPUBufferComp = l_renderingServer->AddGPUBufferComponent("DispatchParamsCBuffer/");
	m_GICBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("GICBuffer/");
	m_animationGPUBufferComp = l_renderingServer->AddGPUBufferComponent("AnimationCBuffer/");
	m_billboardGPUBufferComp = l_renderingServer->AddGPUBufferComponent("BillboardCBuffer/");

	f_sceneLoadingFinishedCallback = [&]()
		{
			m_billboardPassDrawCallInfoVector.resize(3);
			m_billboardPassDrawCallInfoVector[0].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
			m_billboardPassDrawCallInfoVector[1].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::POINT_LIGHT);
			m_billboardPassDrawCallInfoVector[2].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::SPHERE_LIGHT);
		};

	g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RenderingContextServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		InitializeHaltonSampler();

		m_perFrameCBs.resize(g_Engine->getRenderingServer()->GetSwapChainImageCount());

		auto l_renderingServer = g_Engine->getRenderingServer();

		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_PerFrameCBufferGPUBufferComp->m_GPUAccessibility = Accessibility::ReadOnly;
		m_PerFrameCBufferGPUBufferComp->m_ElementCount = 1;
		m_PerFrameCBufferGPUBufferComp->m_ElementSize = sizeof(PerFrameConstantBuffer);

		l_renderingServer->Initialize(m_PerFrameCBufferGPUBufferComp);

		m_PerObjectGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_PerObjectGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_PerObjectGPUBufferComp->m_ElementSize = sizeof(PerObjectConstantBuffer);

		l_renderingServer->Initialize(m_PerObjectGPUBufferComp);

		m_MaterialGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_MaterialGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMaterials;
		m_MaterialGPUBufferComp->m_ElementSize = sizeof(MaterialConstantBuffer);

		l_renderingServer->Initialize(m_MaterialGPUBufferComp);

		m_PointLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxPointLights;
		m_PointLightGPUBufferComp->m_ElementSize = sizeof(PointLightConstantBuffer);

		l_renderingServer->Initialize(m_PointLightGPUBufferComp);

		m_SphereLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxSphereLights;
		m_SphereLightGPUBufferComp->m_ElementSize = sizeof(SphereLightConstantBuffer);

		l_renderingServer->Initialize(m_SphereLightGPUBufferComp);

		m_CSMGPUBufferComp->m_ElementCount = l_RenderingCapability.maxCSMSplits;
		m_CSMGPUBufferComp->m_ElementSize = sizeof(CSMConstantBuffer);

		l_renderingServer->Initialize(m_CSMGPUBufferComp);

		// @TODO: get rid of hard-code stuffs
		m_DispatchParamsGPUBufferComp->m_ElementCount = 8;
		m_DispatchParamsGPUBufferComp->m_ElementSize = sizeof(DispatchParamsConstantBuffer);

		l_renderingServer->Initialize(m_DispatchParamsGPUBufferComp);

		m_GICBufferGPUBufferComp->m_ElementSize = sizeof(GIConstantBuffer);
		m_GICBufferGPUBufferComp->m_ElementCount = 1;

		l_renderingServer->Initialize(m_GICBufferGPUBufferComp);

		m_animationGPUBufferComp->m_ElementCount = 512;
		m_animationGPUBufferComp->m_ElementSize = sizeof(AnimationConstantBuffer);

		l_renderingServer->Initialize(m_animationGPUBufferComp);

		m_billboardGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_billboardGPUBufferComp->m_ElementSize = sizeof(PerObjectConstantBuffer);
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

bool RenderingContextServiceImpl::UpdatePerFrameConstantBuffer()
{
	auto l_camera = static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->GetActiveCamera();
	if (l_camera == nullptr)
		return false;

	auto l_cameraTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_camera->m_Owner);
	if (l_cameraTransformComponent == nullptr)
		return false;

	auto l_p = l_camera->m_projectionMatrix;

	PerFrameConstantBuffer l_perFrameCB = {};
	l_perFrameCB.p_original = l_p;
	l_perFrameCB.p_jittered = l_p;

	auto l_renderingConfigurationService = g_Engine->Get<RenderingConfigurationService>();
	auto l_renderingConfig = l_renderingConfigurationService->GetRenderingConfig();
	auto l_screenResolution = l_renderingConfigurationService->GetScreenResolution();
	if (l_renderingConfig.useTAA)
	{
		auto& l_HaltonSampler = m_HaltonSamplers[g_Engine->getRenderingServer()->GetCurrentFrame()];
	
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = l_HaltonSampler.m_CurrentStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		l_perFrameCB.p_jittered.m02 = l_HaltonSampler.m_Values[l_currentHaltonStep].x / l_screenResolution.x;
		l_perFrameCB.p_jittered.m12 = l_HaltonSampler.m_Values[l_currentHaltonStep].y / l_screenResolution.y;
		l_currentHaltonStep += 1;
	}

	auto r = Math::getInvertRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);

	auto t = Math::getInvertTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);

	l_perFrameCB.camera_posWS = l_cameraTransformComponent->m_globalTransformVector.m_pos;

	l_perFrameCB.v = r * t;

	auto r_prev = l_cameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	auto t_prev = l_cameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	l_perFrameCB.v_prev = r_prev * t_prev;

	l_perFrameCB.zNear = l_camera->m_zNear;
	l_perFrameCB.zFar = l_camera->m_zFar;

	l_perFrameCB.p_inv = l_p.inverse();
	l_perFrameCB.v_inv = l_perFrameCB.v.inverse();
	l_perFrameCB.viewportSize.x = (float)l_screenResolution.x;
	l_perFrameCB.viewportSize.y = (float)l_screenResolution.y;
	l_perFrameCB.minLogLuminance = -10.0f;
	l_perFrameCB.maxLogLuminance = 16.0f;
	l_perFrameCB.aperture = l_camera->m_aperture;
	l_perFrameCB.shutterTime = l_camera->m_shutterTime;
	l_perFrameCB.ISO = l_camera->m_ISO;

	auto l_sun = g_Engine->Get<ComponentManager>()->Get<LightComponent>(0);
	if (l_sun == nullptr)
		return false;

	auto l_sunTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_sun->m_Owner);
	if (l_sunTransformComponent == nullptr)
		return false;

	l_perFrameCB.sun_direction = Math::getDirection(Direction::Backward, l_sunTransformComponent->m_globalTransformVector.m_rot);
	l_perFrameCB.sun_illuminance = l_sun->m_RGBColor * l_sun->m_LuminousFlux;

	static uint32_t currentCascade = 0;
	auto l_renderingCapability = l_renderingConfigurationService->GetRenderingCapability();
	currentCascade = currentCascade < l_renderingCapability.maxCSMSplits - 1 ? ++currentCascade : 0;
	l_perFrameCB.activeCascade = currentCascade;

	auto& l_SplitAABB = l_sun->m_SplitAABBWS;
	auto& l_ViewMatrices = l_sun->m_ViewMatrices;
	auto& l_ProjectionMatrices = l_sun->m_ProjectionMatrices;

	m_CSMCBVector.clear();

	if (l_SplitAABB.size() > 0 && l_ViewMatrices.size() > 0 && l_ProjectionMatrices.size() > 0)
	{
		for (size_t j = 0; j < l_SplitAABB.size(); j++)
		{
			CSMConstantBuffer l_CSMCB;

			l_CSMCB.p = l_ProjectionMatrices[j];
			l_CSMCB.v = l_ViewMatrices[j];

			l_CSMCB.AABBMax = l_SplitAABB[j].m_boundMax;
			l_CSMCB.AABBMin = l_SplitAABB[j].m_boundMin;

			m_CSMCBVector.emplace_back(l_CSMCB);
		}
	}

	m_perFrameCBs[g_Engine->getRenderingServer()->GetCurrentFrame()] = l_perFrameCB;
	
	return true;
}

bool RenderingContextServiceImpl::UpdateLightData()
{
	m_pointLightCBVector.clear();
	m_sphereLightCBVector.clear();

	auto& l_lightComponents = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();
	auto l_lightComponentCount = l_lightComponents.size();

	if (l_lightComponentCount == 0)
		return false;

	for (size_t i = 0; i < l_lightComponentCount; i++)
	{
		auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_lightComponents[i]->m_Owner);
		if (l_transformComponent == nullptr)
			continue;

		if (l_lightComponents[i]->m_LightType == LightType::Point)
		{
			PointLightConstantBuffer l_data;
			l_data.pos = l_transformComponent->m_globalTransformVector.m_pos;
			l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
			l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
			m_pointLightCBVector.emplace_back(l_data);
		}
		else if (l_lightComponents[i]->m_LightType == LightType::Sphere)
		{
			SphereLightConstantBuffer l_data;
			l_data.pos = l_transformComponent->m_globalTransformVector.m_pos;
			l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
			l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
			m_sphereLightCBVector.emplace_back(l_data);
		}
	}

	return true;
}

bool RenderingContextServiceImpl::UpdateDrawCalls()
{
	m_drawCallInfoVector.clear();
	m_perObjectCBVector.clear();
	m_materialCBVector.clear();
	m_animationDrawCallInfoVector.clear();
	m_animationCBVector.clear();

	uint32_t l_drawCallIndex = 0;
	auto& l_cullingResults = g_Engine->Get<PhysicsSimulationService>()->GetCullingResult();
	auto l_cullingResultCount = l_cullingResults.size();
	for (size_t i = 0; i < l_cullingResultCount; i++)
	{
		auto& l_cullingResult = l_cullingResults[i];
		if (l_cullingResult.m_CollisionComponent == nullptr)
			continue;

		auto l_modelComponent = l_cullingResult.m_CollisionComponent->m_ModelComponent;
		if (l_modelComponent == nullptr)
			continue;

		if (l_modelComponent->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_modelComponent->m_Owner);
		if (l_transformComponent == nullptr)
			continue;

		if (l_transformComponent->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		auto l_renderableSet = l_cullingResult.m_CollisionComponent->m_RenderableSet;
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

		DrawCallInfo l_drawCallInfo = {};

		l_drawCallInfo.mesh = l_mesh;
		l_drawCallInfo.m_PerObjectConstantBufferIndex = l_drawCallIndex;
		l_drawCallInfo.meshUsage = l_modelComponent->m_meshUsage;
		l_drawCallInfo.m_VisibilityMask = l_cullingResult.m_VisibilityMask;

		PerObjectConstantBuffer l_perObjectCB = {};
		l_perObjectCB.m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
		l_perObjectCB.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
		l_perObjectCB.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
		l_perObjectCB.UUID = (float)l_transformComponent->m_UUID;
		l_perObjectCB.m_MaterialIndex = l_drawCallIndex; // @TODO: The material is duplicated per object, this should be fixed

		m_perObjectCBVector.emplace_back(l_perObjectCB);

		MaterialConstantBuffer l_materialCB = {};
		l_materialCB.m_MaterialAttributes = l_material->m_materialAttributes;

		auto l_renderingServer = g_Engine->getRenderingServer();
		for (size_t i = 0; i < MaxTextureSlotCount; i++)
		{
			auto l_texture = l_material->m_TextureSlots[i].m_Texture;
			if (l_texture != nullptr)
				l_materialCB.m_TextureIndices[i] = l_renderingServer->GetIndex(l_texture, Accessibility::ReadOnly);
		}

		m_materialCBVector.emplace_back(l_materialCB);
		m_drawCallInfoVector.emplace_back(l_drawCallInfo);
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

			m_animationCBVector.emplace_back(l_animationCB);

			animationDrawCallInfo.animationConstantBufferIndex = (uint32_t)m_animationCBVector.size();
			m_animationDrawCallInfoVector.emplace_back(animationDrawCallInfo);
		}
	}

	// @TODO: use GPU to do OIT

	return true;
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
		PerObjectConstantBuffer l_meshCB;

		auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(i->m_Owner);
		if (l_transformComponent != nullptr)
		{
			l_meshCB.m = Math::toTranslationMatrix(l_transformComponent->m_globalTransformVector.m_pos);
		}

		switch (i->m_LightType)
		{
		case LightType::Directional:
			m_directionalLightPerObjectCB.emplace_back(l_meshCB);
			m_billboardPassDrawCallInfoVector[0].instanceCount++;
			break;
		case LightType::Point:
			m_pointLightPerObjectCB.emplace_back(l_meshCB);
			m_billboardPassDrawCallInfoVector[1].instanceCount++;
			break;
		case LightType::Spot:
			break;
		case LightType::Sphere:
			m_sphereLightPerObjectCB.emplace_back(l_meshCB);
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

	l_renderingServer->Upload(m_PerFrameCBufferGPUBufferComp, &m_perFrameCBs[g_Engine->getRenderingServer()->GetCurrentFrame()]);

	if (m_perObjectCBVector.size() > 0)
	{
		l_renderingServer->Upload(m_PerObjectGPUBufferComp, m_perObjectCBVector, 0, m_perObjectCBVector.size());
	}
	if (m_materialCBVector.size() > 0)
	{
		l_renderingServer->Upload(m_MaterialGPUBufferComp, m_materialCBVector, 0, m_materialCBVector.size());
	}
	if (m_pointLightCBVector.size() > 0)
	{
		l_renderingServer->Upload(m_PointLightGPUBufferComp, m_pointLightCBVector, 0, m_pointLightCBVector.size());
	}
	if (m_sphereLightCBVector.size() > 0)
	{
		l_renderingServer->Upload(m_SphereLightGPUBufferComp, m_sphereLightCBVector, 0, m_sphereLightCBVector.size());
	}
	if (m_CSMCBVector.size() > 0)
	{
		l_renderingServer->Upload(m_CSMGPUBufferComp, m_CSMCBVector, 0, m_CSMCBVector.size());
	}
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

		UpdatePerFrameConstantBuffer();

		UpdateLightData();

		UpdateDrawCalls();

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

	l_renderingServer->Delete(m_PerFrameCBufferGPUBufferComp);
	l_renderingServer->Delete(m_PerObjectGPUBufferComp);
	l_renderingServer->Delete(m_MaterialGPUBufferComp);
	l_renderingServer->Delete(m_PointLightGPUBufferComp);
	l_renderingServer->Delete(m_SphereLightGPUBufferComp);
	l_renderingServer->Delete(m_CSMGPUBufferComp);
	l_renderingServer->Delete(m_DispatchParamsGPUBufferComp);
	l_renderingServer->Delete(m_GICBufferGPUBufferComp);
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
	case GPUBufferUsageType::PerFrame: l_result = m_Impl->m_PerFrameCBufferGPUBufferComp;
		break;
	case GPUBufferUsageType::Mesh: l_result = m_Impl->m_PerObjectGPUBufferComp;
		break;
	case GPUBufferUsageType::Material: l_result = m_Impl->m_MaterialGPUBufferComp;
		break;
	case GPUBufferUsageType::PointLight: l_result = m_Impl->m_PointLightGPUBufferComp;
		break;
	case GPUBufferUsageType::SphereLight: l_result = m_Impl->m_SphereLightGPUBufferComp;
		break;
	case GPUBufferUsageType::CSM: l_result = m_Impl->m_CSMGPUBufferComp;
		break;
	case GPUBufferUsageType::ComputeDispatchParam: l_result = m_Impl->m_DispatchParamsGPUBufferComp;
		break;
	case GPUBufferUsageType::GI:l_result = m_Impl->m_GICBufferGPUBufferComp;
		break;
	case GPUBufferUsageType::Animation: l_result = m_Impl->m_animationGPUBufferComp;
		break;
	case GPUBufferUsageType::Billboard: l_result = m_Impl->m_billboardGPUBufferComp;
		break;
	default:
		break;
	}

	return l_result;
}

const PerFrameConstantBuffer& RenderingContextService::GetPerFrameConstantBuffer()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_perFrameCBs[g_Engine->getRenderingServer()->GetCurrentFrame()];
}

const std::vector<DrawCallInfo>& RenderingContextService::GetDrawCallInfo()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_drawCallInfoVector;
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