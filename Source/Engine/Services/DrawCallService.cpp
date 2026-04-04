#include "DrawCallService.h"

#include "../Common/LogService.h"
#include "EntityRegistry.h"
#include "RenderingConfigurationService.h"
#include "AssetService.h"
#include "../Component/MeshComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/WorldTransformComponent.h"
#include "../Component/VisibilityComponent.h"
#include "../Engine.h"
#include "GraphicsResourceService.h"
#include "FrameManagementService.h"

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
	auto l_frameCount = g_Engine->Get<FrameManagementService>()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformBufferComp : m_TransformPrevBufferComp;
}

GPUBufferComponent* DrawCallServiceImpl::GetPreviousFrameTransformBuffer()
{
	auto l_frameCount = g_Engine->Get<FrameManagementService>()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformPrevBufferComp : m_TransformBufferComp;
}

bool DrawCallServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

	m_GPUModelDataBufferComp = l_rsService->AddGPUBufferComponent("GPUModelDataBuffer/");
	m_TransformBufferComp = l_rsService->AddGPUBufferComponent("TransformBuffer/");
	m_TransformPrevBufferComp = l_rsService->AddGPUBufferComponent("TransformPrevBuffer/");
	m_MaterialGPUBufferComp = l_rsService->AddGPUBufferComponent("MaterialCBuffer/");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool DrawCallServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_GPUModelDataBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_GPUModelDataBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_GPUModelDataBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_GPUModelDataBufferComp->m_ElementSize = sizeof(GPUModelData);

		l_rsService->Initialize(m_GPUModelDataBufferComp);

		m_TransformBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_TransformBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_TransformBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_TransformBufferComp->m_ElementSize = sizeof(TransformConstantBuffer);

		l_rsService->Initialize(m_TransformBufferComp);

		m_TransformPrevBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_TransformPrevBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_TransformPrevBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_TransformPrevBufferComp->m_ElementSize = sizeof(TransformConstantBuffer);

		l_rsService->Initialize(m_TransformPrevBufferComp);

		m_MaterialGPUBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_MaterialGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_MaterialGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMaterials;
		m_MaterialGPUBufferComp->m_ElementSize = sizeof(MaterialConstantBuffer);

		l_rsService->Initialize(m_MaterialGPUBufferComp);

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

	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_MeshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_Meshes = l_MeshStorage.All();
	const auto& l_Owners = l_MeshStorage.AllOwners();
	uint32_t l_drawCallIndex = 0;
	for (size_t i = 0; i < l_Meshes.size(); i++)
	{
		EntityID l_Entity = l_Owners[i];
		const MeshComponent& l_mesh = l_Meshes[i];

		if (l_mesh.m_ObjectStatus != ObjectStatus::Activated)
			continue;

		auto* l_resource = AssetService::GetMeshAsset(l_mesh.m_Asset);
		if (!l_resource || l_resource->m_Residency != AssetResidency::Resident)
			continue;

		auto* l_material = l_registry->Get<MaterialComponent>(l_Entity);
		if (!l_material)
			continue;

		auto* l_vis = l_registry->Get<VisibilityComponent>(l_Entity);
		if (l_vis && !l_vis->m_Visible)
			continue;

		GPUModelData l_gpuModelData = {};

		l_gpuModelData.m_VertexBufferAddress = l_resource->m_VertexBufferView.m_BufferLocation;
		l_gpuModelData.m_IndexBufferAddress = l_resource->m_IndexBufferView.m_BufferLocation;

		if (l_resource->m_VertexBufferView.m_StrideInBytes == 0)
		{
			Log(Error, "Vertex stride is zero - cannot calculate vertex count");
			l_gpuModelData.m_VertexCount = 0;
		}
		else
		{
			l_gpuModelData.m_VertexCount = l_resource->m_VertexBufferView.m_SizeInBytes / l_resource->m_VertexBufferView.m_StrideInBytes;
		}
		l_gpuModelData.m_IndexCount = l_resource->GetIndexCount();
		l_gpuModelData.m_VertexStride = l_resource->m_VertexBufferView.m_StrideInBytes;
		l_gpuModelData.m_IndexStride = l_resource->m_IndexBufferView.m_StrideInBytes;

		l_gpuModelData.m_MaterialIndex = l_drawCallIndex;
		l_gpuModelData.m_UUID = static_cast<float>(l_Entity);

		l_gpuModelData.m_VisibilityMask = static_cast<uint32_t>(VisibilityMask::MainCamera);
		l_gpuModelData.m_MeshUsage = static_cast<uint32_t>(MeshUsage::Static);

		auto* l_world = l_registry->Get<WorldTransformComponent>(l_Entity);
		const AABB& l_localAabb = l_vis ? l_vis->m_AABB : l_resource->m_AABB;
		if (l_world)
		{
			const auto& M = l_world->m_WorldMatrix;
			Vec4 l_corners[8] = {
				Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMin.z, 1.0f),
				Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMin.z, 1.0f),
				Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMin.z, 1.0f),
				Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMin.z, 1.0f),
				Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMax.z, 1.0f),
				Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMax.z, 1.0f),
				Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMax.z, 1.0f),
				Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMax.z, 1.0f),
			};
			Vec4 wsMin = Math::maxVec4<float>;
			Vec4 wsMax = Math::minVec4<float>;
			for (int i = 0; i < 8; i++)
			{
				Vec4 ws = M * l_corners[i];
				wsMin = Math::elementWiseMin(wsMin, ws);
				wsMax = Math::elementWiseMax(wsMax, ws);
			}
			wsMin.w = 1.0f;
			wsMax.w = 1.0f;
			l_gpuModelData.m_BoundingBoxMin = wsMin;
			l_gpuModelData.m_BoundingBoxMax = wsMax;
		}
		else
		{
			l_gpuModelData.m_BoundingBoxMin = Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMin.z, 1.0f);
			l_gpuModelData.m_BoundingBoxMax = Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMax.z, 1.0f);
		}

		l_gpuModelData.m_InstanceCount = 1;
		l_gpuModelData.m_FirstInstance = 0;

		m_GPUModelDataVector.emplace_back(l_gpuModelData);

		TransformConstantBuffer l_transformCB = {};
		if (l_world)
		{
			l_transformCB.m = l_world->m_WorldMatrix;
			l_transformCB.normalMat = l_world->m_WorldRotationMatrix;
		}
		m_TransformBufferVector.emplace_back(l_transformCB);

		MaterialConstantBuffer l_materialCB = {};
		auto* l_materialAsset = AssetService::GetMaterialAsset(l_material->m_Asset);

		if (l_materialAsset)
			l_materialCB.m_MaterialAttributes = l_materialAsset->m_Attributes;

		for (size_t j = 0; j < MaxTextureSlotCount; j++)
		{
			l_materialCB.m_TextureIndices[j] = INVALID_TEXTURE_INDEX;
		}

		if (l_materialAsset)
		{
			for (size_t j = 0; j < l_materialAsset->m_TextureNames.size(); j++)
			{
				const auto& l_textureName = l_materialAsset->m_TextureNames[j];
				if (l_textureName.empty())
					continue;

				auto l_texture = l_rsService->FindTextureByName(l_textureName.c_str());
				if (!l_texture || l_texture->m_ObjectStatus != ObjectStatus::Activated)
					continue;

				auto textureIndex = l_rsService->GetIndex(l_texture, Accessibility::ReadOnly);
				l_materialCB.m_TextureIndices[j] = textureIndex.value_or(INVALID_TEXTURE_INDEX);
			}
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

	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

		if (m_GPUModelDataVector.size() > 0)
		{
			l_rsService->Upload(m_GPUModelDataBufferComp, m_GPUModelDataVector, 0, m_GPUModelDataVector.size());
		}
		if (m_TransformBufferVector.size() > 0)
		{
			auto l_currentFrameTransformBuffer = GetCurrentFrameTransformBuffer();
			l_rsService->Upload(l_currentFrameTransformBuffer, m_TransformBufferVector, 0, m_TransformBufferVector.size());
		}
		if (m_MaterialCBVector.size() > 0)
		{
			l_rsService->Upload(m_MaterialGPUBufferComp, m_MaterialCBVector, 0, m_MaterialCBVector.size());
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
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

	l_rsService->Delete(m_GPUModelDataBufferComp);
	l_rsService->Delete(m_TransformBufferComp);
	l_rsService->Delete(m_TransformPrevBufferComp);
	l_rsService->Delete(m_MaterialGPUBufferComp);

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
