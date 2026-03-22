#include "DrawCallService.h"

#include "../Common/LogService.h"
#include "EntityRegistry.h"
#include "RenderingConfigurationService.h"
#include "../Component/MeshComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/WorldTransformComponent.h"
#include "../Component/VisibilityComponent.h"
#include "../Engine.h"

using namespace Inno;

namespace Inno
{
	struct DrawCallServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		mutable std::shared_mutex m_Mutex;

		std::vector<GPUModelData> m_GPUModelDataVector;
		std::vector<TransformConstantBuffer> m_TransformBufferVector;
		std::vector<MaterialConstantBuffer> m_MaterialCBVector;

		GPUBufferComponent* m_GPUModelDataBufferComp;
		GPUBufferComponent* m_TransformBufferComp;
		GPUBufferComponent* m_TransformPrevBufferComp;
		GPUBufferComponent* m_MaterialGPUBufferComp;

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateDrawCalls();

		GPUBufferComponent* GetCurrentFrameTransformBuffer();
		GPUBufferComponent* GetPreviousFrameTransformBuffer();
	};
}

GPUBufferComponent* DrawCallServiceImpl::GetCurrentFrameTransformBuffer()
{
	auto l_frameCount = g_Engine->getGraphicsService()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformBufferComp : m_TransformPrevBufferComp;
}

GPUBufferComponent* DrawCallServiceImpl::GetPreviousFrameTransformBuffer()
{
	auto l_frameCount = g_Engine->getGraphicsService()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformPrevBufferComp : m_TransformBufferComp;
}

bool DrawCallServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getGraphicsService();

	m_GPUModelDataBufferComp = l_renderingServer->AddGPUBufferComponent("GPUModelDataBuffer/");
	m_TransformBufferComp = l_renderingServer->AddGPUBufferComponent("TransformBuffer/");
	m_TransformPrevBufferComp = l_renderingServer->AddGPUBufferComponent("TransformPrevBuffer/");
	m_MaterialGPUBufferComp = l_renderingServer->AddGPUBufferComponent("MaterialCBuffer/");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool DrawCallServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		auto l_renderingServer = g_Engine->getGraphicsService();

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

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "DrawCallService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "DrawCallService is not created!");
		return false;
	}
}

bool DrawCallServiceImpl::UpdateDrawCalls()
{
	m_GPUModelDataVector.clear();
	m_TransformBufferVector.clear();
	m_MaterialCBVector.clear();

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_MeshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_Meshes = l_MeshStorage.All();
	const auto& l_Owners = l_MeshStorage.AllOwners();

	uint32_t l_drawCallIndex = 0;
	for (size_t i = 0; i < l_Meshes.size(); i++)
	{
		EntityID l_Entity = l_Owners[i];
		const MeshComponent& l_mesh = l_Meshes[i];

		auto* l_material = l_registry->Get<MaterialComponent>(l_Entity);
		if (!l_material)
			continue;

		auto* l_vis = l_registry->Get<VisibilityComponent>(l_Entity);
		if (l_vis && !l_vis->m_Visible)
			continue;

		GPUModelData l_gpuModelData = {};

		l_gpuModelData.m_VertexBufferAddress = l_mesh.m_VertexBufferView.m_BufferLocation;
		l_gpuModelData.m_IndexBufferAddress = l_mesh.m_IndexBufferView.m_BufferLocation;

		if (l_mesh.m_VertexBufferView.m_StrideInBytes == 0)
		{
			Log(Error, "Vertex stride is zero - cannot calculate vertex count");
			l_gpuModelData.m_VertexCount = 0;
		}
		else
		{
			l_gpuModelData.m_VertexCount = l_mesh.m_VertexBufferView.m_SizeInBytes / l_mesh.m_VertexBufferView.m_StrideInBytes;
		}
		l_gpuModelData.m_IndexCount = l_mesh.GetIndexCount();
		l_gpuModelData.m_VertexStride = l_mesh.m_VertexBufferView.m_StrideInBytes;
		l_gpuModelData.m_IndexStride = l_mesh.m_IndexBufferView.m_StrideInBytes;

		l_gpuModelData.m_MaterialIndex = l_drawCallIndex;
		l_gpuModelData.m_UUID = static_cast<float>(l_Entity);

		l_gpuModelData.m_VisibilityMask = static_cast<uint32_t>(VisibilityMask::MainCamera);
		l_gpuModelData.m_MeshUsage = static_cast<uint32_t>(MeshUsage::Static);

		const AABB& l_aabb = l_vis ? l_vis->m_AABB : l_mesh.m_AABB;
		l_gpuModelData.m_BoundingBoxMin = Vec4(l_aabb.m_boundMin.x, l_aabb.m_boundMin.y, l_aabb.m_boundMin.z, 1.0f);
		l_gpuModelData.m_BoundingBoxMax = Vec4(l_aabb.m_boundMax.x, l_aabb.m_boundMax.y, l_aabb.m_boundMax.z, 1.0f);

		l_gpuModelData.m_InstanceCount = 1;
		l_gpuModelData.m_FirstInstance = 0;

		m_GPUModelDataVector.emplace_back(l_gpuModelData);

		auto* l_world = l_registry->Get<WorldTransformComponent>(l_Entity);
		TransformConstantBuffer l_transformCB = {};
		if (l_world)
		{
			l_transformCB.m = l_world->m_WorldMatrix;
			l_transformCB.normalMat = l_world->m_WorldRotationMatrix;
		}
		m_TransformBufferVector.emplace_back(l_transformCB);

		MaterialConstantBuffer l_materialCB = {};
		l_materialCB.m_MaterialAttributes = l_material->m_materialAttributes;

		for (size_t j = 0; j < MaxTextureSlotCount; j++)
		{
			l_materialCB.m_TextureIndices[j] = INVALID_TEXTURE_INDEX;
		}

		for (size_t j = 0; j < l_material->m_TextureComponents.size(); j++)
		{
			auto l_textureID = l_material->m_TextureComponents[j];
			if (l_textureID.empty())
				continue;

			// TODO Phase2-migrate: TextureComponent not yet in EntityRegistry - will migrate in Task 9
			// if (!l_texture || l_texture->m_ObjectStatus != ObjectStatus::Activated)
			// 	continue;
			// auto textureIndex = l_renderingServer->GetIndex(l_texture, Accessibility::ReadOnly);
			// l_materialCB.m_TextureIndices[j] = textureIndex.value_or(INVALID_TEXTURE_INDEX);
		}

		m_MaterialCBVector.emplace_back(l_materialCB);
		l_drawCallIndex++;
	}

	return true;
}

bool DrawCallServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		std::lock_guard<std::shared_mutex> l_lock(m_Mutex);

		UpdateDrawCalls();

		auto l_renderingServer = g_Engine->getGraphicsService();

		if (m_GPUModelDataVector.size() > 0)
		{
			l_renderingServer->Upload(m_GPUModelDataBufferComp, m_GPUModelDataVector, 0, m_GPUModelDataVector.size());
		}
		if (m_TransformBufferVector.size() > 0)
		{
			auto l_currentFrameTransformBuffer = GetCurrentFrameTransformBuffer();
			l_renderingServer->Upload(l_currentFrameTransformBuffer, m_TransformBufferVector, 0, m_TransformBufferVector.size());
		}
		if (m_MaterialCBVector.size() > 0)
		{
			l_renderingServer->Upload(m_MaterialGPUBufferComp, m_MaterialCBVector, 0, m_MaterialCBVector.size());
		}

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool DrawCallServiceImpl::Terminate()
{
	auto l_renderingServer = g_Engine->getGraphicsService();

	l_renderingServer->Delete(m_GPUModelDataBufferComp);
	l_renderingServer->Delete(m_TransformBufferComp);
	l_renderingServer->Delete(m_TransformPrevBufferComp);
	l_renderingServer->Delete(m_MaterialGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "DrawCallService has been terminated.");
	return true;
}

bool DrawCallService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new DrawCallServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool DrawCallService::Initialize()
{
	return m_Impl->Initialize();
}

bool DrawCallService::Update()
{
	return m_Impl->Update();
}

bool DrawCallService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus DrawCallService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

const std::vector<GPUModelData>& DrawCallService::GetGPUModelData()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_GPUModelDataVector;
}

GPUBufferComponent* DrawCallService::GetGPUModelDataBuffer()
{
	return m_Impl->m_GPUModelDataBufferComp;
}

GPUBufferComponent* DrawCallService::GetCurrentFrameTransformBuffer()
{
	return m_Impl->GetCurrentFrameTransformBuffer();
}

GPUBufferComponent* DrawCallService::GetPreviousFrameTransformBuffer()
{
	return m_Impl->GetPreviousFrameTransformBuffer();
}

GPUBufferComponent* DrawCallService::GetMaterialBuffer()
{
	return m_Impl->m_MaterialGPUBufferComp;
}
