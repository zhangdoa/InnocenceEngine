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

		std::vector<GPUModelData> m_gpuModelDataVector;
		std::vector<TransformConstantBuffer> m_transformBufferVector;
		std::vector<MaterialConstantBuffer> m_materialCBVector;

		std::vector<AnimationDrawCallInfo> m_animationDrawCallInfoVector;
		std::vector<AnimationConstantBuffer> m_animationCBVector;

		std::vector<TransformConstantBuffer> m_directionalLightPerObjectCB;
		std::vector<TransformConstantBuffer> m_pointLightPerObjectCB;
		std::vector<TransformConstantBuffer> m_sphereLightPerObjectCB;

		std::vector<BillboardPassDrawCallInfo> m_billboardPassDrawCallInfoVector;
		std::vector<TransformConstantBuffer> m_billboardPassPerObjectCB;

		std::vector<DebugPassDrawCallInfo> m_debugPassDrawCallInfoVector;
		std::vector<TransformConstantBuffer> m_debugPassPerObjectCB;

		GPUBufferComponent* m_GPUModelDataBufferComp;
		GPUBufferComponent* m_TransformBufferComp;
		GPUBufferComponent* m_TransformPrevBufferComp;
		GPUBufferComponent* m_MaterialGPUBufferComp;
		GPUBufferComponent* m_animationGPUBufferComp;
		GPUBufferComponent* m_billboardGPUBufferComp;

		std::function<void()> f_sceneLoadingFinishedCallback;

		bool Setup(ISystemConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateDrawCalls();
		bool UpdateBillboardPassData();
		bool UpdateDebuggerPassData();
		bool UploadGPUBuffers();

		GPUBufferComponent* GetCurrentFrameTransformBuffer();
		GPUBufferComponent* GetPreviousFrameTransformBuffer();
	};
}

GPUBufferComponent* RenderingContextServiceImpl::GetCurrentFrameTransformBuffer()
{
	auto l_frameCount = g_Engine->getRenderingServer()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformBufferComp : m_TransformPrevBufferComp;
}

GPUBufferComponent* RenderingContextServiceImpl::GetPreviousFrameTransformBuffer()
{
	auto l_frameCount = g_Engine->getRenderingServer()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformPrevBufferComp : m_TransformBufferComp;
}


bool RenderingContextServiceImpl::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_GPUModelDataBufferComp = l_renderingServer->AddGPUBufferComponent("GPUModelDataBuffer/");
	m_TransformBufferComp = l_renderingServer->AddGPUBufferComponent("TransformBuffer/");
	m_TransformPrevBufferComp = l_renderingServer->AddGPUBufferComponent("TransformPrevBuffer/");
	m_MaterialGPUBufferComp = l_renderingServer->AddGPUBufferComponent("MaterialCBuffer/");
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

		m_GPUModelDataBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_GPUModelDataBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_GPUModelDataBufferComp->m_ElementSize = sizeof(GPUModelData);

		l_renderingServer->Initialize(m_GPUModelDataBufferComp);

		m_TransformBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_TransformBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_TransformBufferComp->m_ElementSize = sizeof(TransformConstantBuffer);

		l_renderingServer->Initialize(m_TransformBufferComp);

		m_TransformPrevBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_TransformPrevBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_TransformPrevBufferComp->m_ElementSize = sizeof(TransformConstantBuffer);

		l_renderingServer->Initialize(m_TransformPrevBufferComp);

		m_MaterialGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_MaterialGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMaterials;
		m_MaterialGPUBufferComp->m_ElementSize = sizeof(MaterialConstantBuffer);

		l_renderingServer->Initialize(m_MaterialGPUBufferComp);

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

bool RenderingContextServiceImpl::UpdateDrawCalls()
{
	m_gpuModelDataVector.clear();
	m_transformBufferVector.clear();
	m_materialCBVector.clear();
	m_animationDrawCallInfoVector.clear();
	m_animationCBVector.clear();

	uint32_t l_drawCallIndex = 0;
	auto& l_modelComponents = g_Engine->Get<ComponentManager>()->GetAll<ModelComponent>();
	for (auto l_modelComponent : l_modelComponents)
	{
		if (!l_modelComponent || l_modelComponent->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		for (auto l_drawCallComponentID : l_modelComponent->m_DrawCallComponents)
		{
			if (!l_drawCallComponentID)
				continue;

			auto l_drawCallComponent = g_Engine->Get<ComponentManager>()->FindByUUID<DrawCallComponent>(l_drawCallComponentID);
			if (!l_drawCallComponent || l_drawCallComponent->m_ObjectStatus != ObjectStatus::Activated)
				continue;

			auto l_meshID = l_drawCallComponent->m_MeshComponent;
			if (!l_meshID)
				continue;

			auto l_materialID = l_drawCallComponent->m_MaterialComponent;
			if (!l_materialID)
				continue;

			auto l_mesh = g_Engine->Get<ComponentManager>()->FindByUUID<MeshComponent>(l_meshID);
			if (!l_mesh || l_mesh->m_ObjectStatus != ObjectStatus::Activated)
				continue;

			auto l_material = g_Engine->Get<ComponentManager>()->FindByUUID<MaterialComponent>(l_materialID);				
			if (!l_material || l_material->m_ObjectStatus != ObjectStatus::Activated)
				continue;

			GPUModelData l_gpuModelData = {};

			// Vertex and index buffer GPU addresses
			l_gpuModelData.m_VertexBufferAddress = l_mesh->m_VertexBufferView.m_BufferLocation;
			l_gpuModelData.m_IndexBufferAddress = l_mesh->m_IndexBufferView.m_BufferLocation;
			
			// Buffer metadata
			if (l_mesh->m_VertexBufferView.m_StrideInBytes == 0)
			{
				Log(Error, "Vertex stride is zero - cannot calculate vertex count");
				l_gpuModelData.m_VertexCount = 0;
			}
			else
			{
				l_gpuModelData.m_VertexCount = l_mesh->m_VertexBufferView.m_SizeInBytes / l_mesh->m_VertexBufferView.m_StrideInBytes;
			}
			l_gpuModelData.m_IndexCount = l_mesh->GetIndexCount();
			l_gpuModelData.m_VertexStride = l_mesh->m_VertexBufferView.m_StrideInBytes;
			l_gpuModelData.m_IndexStride = l_mesh->m_IndexBufferView.m_StrideInBytes;
			
			// Material and identification
			l_gpuModelData.m_MaterialIndex = l_drawCallIndex;
			l_gpuModelData.m_UUID = (float)l_modelComponent->m_UUID;
			
			// Visibility and culling data
			l_gpuModelData.m_VisibilityMask = static_cast<uint32_t>(VisibilityMask::MainCamera);
			l_gpuModelData.m_MeshUsage = static_cast<uint32_t>(MeshUsage::Static);
			
			// Bounding box for GPU culling
			l_gpuModelData.m_BoundingBoxMin = Vec4(l_modelComponent->m_AABB.m_boundMin.x, l_modelComponent->m_AABB.m_boundMin.y, l_modelComponent->m_AABB.m_boundMin.z, 1.0f);
			l_gpuModelData.m_BoundingBoxMax = Vec4(l_modelComponent->m_AABB.m_boundMax.x, l_modelComponent->m_AABB.m_boundMax.y, l_modelComponent->m_AABB.m_boundMax.z, 1.0f);
			
			// Instance data
			l_gpuModelData.m_InstanceCount = 1;
			l_gpuModelData.m_FirstInstance = 0;

			m_gpuModelDataVector.emplace_back(l_gpuModelData);

			// Create transform buffer data
			TransformConstantBuffer l_transformCB = {};
			l_transformCB.m = l_modelComponent->m_Transform.GetMatrix();
			l_transformCB.normalMat = l_modelComponent->m_Transform.GetRotationMatrix();
			m_transformBufferVector.emplace_back(l_transformCB);

			MaterialConstantBuffer l_materialCB = {};
			l_materialCB.m_MaterialAttributes = l_material->m_materialAttributes;

			auto l_renderingServer = g_Engine->getRenderingServer();
			// Initialize all texture indices to invalid first
			for (size_t i = 0; i < MaxTextureSlotCount; i++)
			{
				l_materialCB.m_TextureIndices[i] = INVALID_TEXTURE_INDEX;
			}

			// Then populate with actual texture components
			for (size_t i = 0; i < l_material->m_TextureComponents.size(); i++)
			{
				auto l_textureID = l_material->m_TextureComponents[i];
				if (!l_textureID)
					continue;

				auto l_texture = g_Engine->Get<ComponentManager>()->FindByUUID<TextureComponent>(l_textureID);
				if (!l_texture || l_texture->m_ObjectStatus != ObjectStatus::Activated)
					continue;

				auto textureIndex = l_renderingServer->GetIndex(l_texture, Accessibility::ReadOnly);
				l_materialCB.m_TextureIndices[i] = textureIndex.value_or(INVALID_TEXTURE_INDEX);
			}

			m_materialCBVector.emplace_back(l_materialCB);
			l_drawCallIndex++;

			// if (l_modelComponent->m_meshUsage == MeshUsage::Skeletal)
			// {
			// 	auto l_result = g_Engine->Get<AnimationService>()->GetAnimationInstance(l_modelComponent->m_UUID);
			// 	if (l_result.animationData.ADC == nullptr)
			// 		continue;

			// 	AnimationDrawCallInfo animationDrawCallInfo;
			// 	animationDrawCallInfo.animationInstance = l_result;
			// 	animationDrawCallInfo.drawCallInfo = l_drawCallInfo;

			// 	AnimationConstantBuffer l_animationCB;
			// 	l_animationCB.duration = animationDrawCallInfo.animationInstance.animationData.ADC->m_Duration;
			// 	l_animationCB.numChannels = animationDrawCallInfo.animationInstance.animationData.ADC->m_NumChannels;
			// 	l_animationCB.numTicks = animationDrawCallInfo.animationInstance.animationData.ADC->m_NumTicks;
			// 	l_animationCB.currentTime = animationDrawCallInfo.animationInstance.currentTime / l_animationCB.duration;
			// 	l_animationCB.rootOffsetMatrix = Math::generateIdentityMatrix<float>();

			// 	m_animationCBVector.emplace_back(l_animationCB);

			// 	animationDrawCallInfo.animationConstantBufferIndex = (uint32_t)m_animationCBVector.size();
			// 	m_animationDrawCallInfoVector.emplace_back(animationDrawCallInfo);
			// }
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

	if (m_gpuModelDataVector.size() > 0)
	{
		l_renderingServer->Upload(m_GPUModelDataBufferComp, m_gpuModelDataVector, 0, m_gpuModelDataVector.size());
	}
	if (m_transformBufferVector.size() > 0)
	{
		auto l_currentFrameTransformBuffer = GetCurrentFrameTransformBuffer();
		l_renderingServer->Upload(l_currentFrameTransformBuffer, m_transformBufferVector, 0, m_transformBufferVector.size());
	}
	if (m_materialCBVector.size() > 0)
	{
		l_renderingServer->Upload(m_MaterialGPUBufferComp, m_materialCBVector, 0, m_materialCBVector.size());
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

	l_renderingServer->Delete(m_GPUModelDataBufferComp);
	l_renderingServer->Delete(m_MaterialGPUBufferComp);
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
	case GPUBufferUsageType::GPUModelData: l_result = m_Impl->m_GPUModelDataBufferComp;
		break;
	case GPUBufferUsageType::Transform: l_result = m_Impl->GetCurrentFrameTransformBuffer();
		break;
	case GPUBufferUsageType::TransformPrev: l_result = m_Impl->GetPreviousFrameTransformBuffer();
		break;
	case GPUBufferUsageType::Material: l_result = m_Impl->m_MaterialGPUBufferComp;
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

const std::vector<GPUModelData>& RenderingContextService::GetGPUModelData()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_gpuModelDataVector;
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