#include "DrawCallService.h"

#include "../Common/LogService.h"
#include "ComponentManager.h"
#include "RenderingConfigurationService.h"
#include "../Component/ModelComponent.h"
#include "../Component/MeshComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/TextureComponent.h"
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

		bool Setup(ISystemConfig* systemConfig);
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
	auto l_frameCount = g_Engine->getRenderingServer()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformBufferComp : m_TransformPrevBufferComp;
}

GPUBufferComponent* DrawCallServiceImpl::GetPreviousFrameTransformBuffer()
{
	auto l_frameCount = g_Engine->getRenderingServer()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformPrevBufferComp : m_TransformBufferComp;
}

bool DrawCallServiceImpl::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

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
			if (!l_mesh)
				continue;

			auto l_material = g_Engine->Get<ComponentManager>()->FindByUUID<MaterialComponent>(l_materialID);
			if (!l_material)
				continue;

			GPUModelData l_gpuModelData = {};

			l_gpuModelData.m_VertexBufferAddress = l_mesh->m_VertexBufferView.m_BufferLocation;
			l_gpuModelData.m_IndexBufferAddress = l_mesh->m_IndexBufferView.m_BufferLocation;

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

			l_gpuModelData.m_MaterialIndex = l_drawCallIndex;
			l_gpuModelData.m_UUID = (float)l_modelComponent->m_UUID;

			l_gpuModelData.m_VisibilityMask = static_cast<uint32_t>(VisibilityMask::MainCamera);
			l_gpuModelData.m_MeshUsage = static_cast<uint32_t>(MeshUsage::Static);

			l_gpuModelData.m_BoundingBoxMin = Vec4(l_modelComponent->m_AABB.m_boundMin.x, l_modelComponent->m_AABB.m_boundMin.y, l_modelComponent->m_AABB.m_boundMin.z, 1.0f);
			l_gpuModelData.m_BoundingBoxMax = Vec4(l_modelComponent->m_AABB.m_boundMax.x, l_modelComponent->m_AABB.m_boundMax.y, l_modelComponent->m_AABB.m_boundMax.z, 1.0f);

			l_gpuModelData.m_InstanceCount = 1;
			l_gpuModelData.m_FirstInstance = 0;

			m_GPUModelDataVector.emplace_back(l_gpuModelData);

			TransformConstantBuffer l_transformCB = {};
			l_transformCB.m = l_modelComponent->m_Transform.GetMatrix();
			l_transformCB.normalMat = l_modelComponent->m_Transform.GetRotationMatrix();
			m_TransformBufferVector.emplace_back(l_transformCB);

			MaterialConstantBuffer l_materialCB = {};
			l_materialCB.m_MaterialAttributes = l_material->m_materialAttributes;

			auto l_renderingServer = g_Engine->getRenderingServer();
			for (size_t i = 0; i < MaxTextureSlotCount; i++)
			{
				l_materialCB.m_TextureIndices[i] = INVALID_TEXTURE_INDEX;
			}

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

			m_MaterialCBVector.emplace_back(l_materialCB);
			l_drawCallIndex++;
		}
	}

	return true;
}

bool DrawCallServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		std::lock_guard<std::shared_mutex> l_lock(m_Mutex);

		UpdateDrawCalls();

		auto l_renderingServer = g_Engine->getRenderingServer();

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
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_GPUModelDataBufferComp);
	l_renderingServer->Delete(m_TransformBufferComp);
	l_renderingServer->Delete(m_TransformPrevBufferComp);
	l_renderingServer->Delete(m_MaterialGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "DrawCallService has been terminated.");
	return true;
}

bool DrawCallService::Setup(ISystemConfig* systemConfig)
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
