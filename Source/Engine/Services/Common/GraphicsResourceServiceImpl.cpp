#include "../GraphicsResourceService.h"
#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/MathHelper.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Services/AssetService.h"

#include "../../Engine.h"

using namespace Inno;

// Static member definitions
Accessibility Accessibility::Immutable = Accessibility(false, false);
Accessibility Accessibility::ReadOnly = Accessibility(true, false);
Accessibility Accessibility::WriteOnly = Accessibility(false, true);
Accessibility Accessibility::ReadWrite = Accessibility(true, true);
Accessibility Accessibility::CopySource = Accessibility(true, false, true, false);
Accessibility Accessibility::CopyDestination = Accessibility(false, true, false, true);

uint32_t GraphicsResourceService::GetCurrentFrameIndex()
{
	return g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
}

bool GraphicsResourceService::Setup(IServiceConfig* systemConfig)
{
	auto l_cap = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();
	m_GPUHandlePools.Meshes         = TObjectPool<MeshComponent>::Create(l_cap.maxMeshes);
	m_GPUHandlePools.Textures       = TObjectPool<TextureComponent>::Create(l_cap.maxTextures);
	m_GPUHandlePools.Materials      = TObjectPool<MaterialComponent>::Create(l_cap.maxMaterials);
	m_GPUHandlePools.RenderPasses   = TObjectPool<RenderPassComponent>::Create(128);
	m_GPUHandlePools.ShaderPrograms = TObjectPool<ShaderProgramComponent>::Create(256);
	m_GPUHandlePools.Samplers       = TObjectPool<SamplerComponent>::Create(256);
	m_GPUHandlePools.GPUBuffers     = TObjectPool<GPUBufferComponent>::Create(l_cap.maxBuffers);
	m_GPUHandlePools.CommandLists   = TObjectPool<CommandListComponent>::Create(256);

	if (!InitializePool())
	{
		Log(Error, "GraphicsResourceService: Failed to initialize backend-specific pools.");
		return false;
	}

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "GraphicsResourceService Setup finished.");
	return true;
}

bool GraphicsResourceService::Initialize()
{
	return true;
}

bool GraphicsResourceService::Terminate()
{
	auto l_result = TerminatePool();

	TObjectPool<MeshComponent>::Destruct(m_GPUHandlePools.Meshes);
	TObjectPool<TextureComponent>::Destruct(m_GPUHandlePools.Textures);
	TObjectPool<MaterialComponent>::Destruct(m_GPUHandlePools.Materials);
	TObjectPool<RenderPassComponent>::Destruct(m_GPUHandlePools.RenderPasses);
	TObjectPool<ShaderProgramComponent>::Destruct(m_GPUHandlePools.ShaderPrograms);
	TObjectPool<SamplerComponent>::Destruct(m_GPUHandlePools.Samplers);
	TObjectPool<GPUBufferComponent>::Destruct(m_GPUHandlePools.GPUBuffers);
	TObjectPool<CommandListComponent>::Destruct(m_GPUHandlePools.CommandLists);

	m_ObjectStatus = ObjectStatus::Terminated;

	if (l_result)
		Log(Success, "GraphicsResourceService has been terminated.");
	else
		Log(Error, "Failed to terminate GraphicsResourceService.");

	return l_result;
}

bool GraphicsResourceService::InitializeImpl(MaterialComponent* material)
{
	return true;
}

template <typename T>
static T* AllocateGPUHandle(TObjectPool<T>* pool,
                             ThreadSafeUnorderedMap<std::string, T*>& lut,
                             ThreadSafeVector<T*>& pointers,
                             const char* name)
{
	if (!name || name[0] == '\0')
	{
		Log(Error, "GPU handle name cannot be empty.");
		return nullptr;
	}

	auto l_existing = lut.find(name);
	if (l_existing != lut.end())
		return l_existing->second;

	auto l_ptr = pool->Spawn();
	if (!l_ptr)
	{
		Log(Error, "GPU handle pool exhausted for name: ", name);
		return nullptr;
	}

	l_ptr->m_ObjectStatus = ObjectStatus::Created;
	l_ptr->m_InstanceName = ObjectName(name);

	lut.emplace(name, l_ptr);
	pointers.emplace_back(l_ptr);
	return l_ptr;
}

MeshComponent* GraphicsResourceService::AddMeshComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.Meshes, m_GPUHandlePools.MeshLUT, m_GPUHandlePools.MeshPointers, name);
}

TextureComponent* GraphicsResourceService::AddTextureComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.Textures, m_GPUHandlePools.TextureLUT, m_GPUHandlePools.TexturePointers, name);
}

MaterialComponent* GraphicsResourceService::AddMaterialComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.Materials, m_GPUHandlePools.MaterialLUT, m_GPUHandlePools.MaterialPointers, name);
}

RenderPassComponent* GraphicsResourceService::AddRenderPassComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.RenderPasses, m_GPUHandlePools.RenderPassLUT, m_GPUHandlePools.RenderPassPointers, name);
}

ShaderProgramComponent* GraphicsResourceService::AddShaderProgramComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.ShaderPrograms, m_GPUHandlePools.ShaderProgramLUT, m_GPUHandlePools.ShaderProgramPointers, name);
}

SamplerComponent* GraphicsResourceService::AddSamplerComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.Samplers, m_GPUHandlePools.SamplerLUT, m_GPUHandlePools.SamplerPointers, name);
}

GPUBufferComponent* GraphicsResourceService::AddGPUBufferComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.GPUBuffers, m_GPUHandlePools.GPUBufferLUT, m_GPUHandlePools.GPUBufferPointers, name);
}

CommandListComponent* GraphicsResourceService::AddCommandListComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.CommandLists, m_GPUHandlePools.CommandListLUT, m_GPUHandlePools.CommandListPointers, name);
}

template <typename T>
static void ReleaseFromPoolStatic(TObjectPool<T>* pool,
                            ThreadSafeUnorderedMap<std::string, T*>& lut,
                            ThreadSafeVector<T*>& pointers,
                            T* ptr)
{
	if (!ptr) return;
	lut.erase(std::string(ptr->m_InstanceName.c_str()));
	pointers.eraseByValue(ptr);
	pool->Destroy(ptr);
}

// Base Delete implementations — call virtual DX12-specific cleanup then pool release
bool GraphicsResourceService::Delete(MeshComponent* mesh)
{
	// DX12-specific cleanup handled by derived class override
	ReleaseFromPoolStatic(m_GPUHandlePools.Meshes, m_GPUHandlePools.MeshLUT, m_GPUHandlePools.MeshPointers, mesh);
	return true;
}

bool GraphicsResourceService::Delete(TextureComponent* texture)
{
	ReleaseFromPoolStatic(m_GPUHandlePools.Textures, m_GPUHandlePools.TextureLUT, m_GPUHandlePools.TexturePointers, texture);
	return true;
}

bool GraphicsResourceService::Delete(MaterialComponent* material)
{
	m_initializedMaterials.erase(material);
	ReleaseFromPoolStatic(m_GPUHandlePools.Materials, m_GPUHandlePools.MaterialLUT, m_GPUHandlePools.MaterialPointers, material);
	return true;
}

bool GraphicsResourceService::Delete(RenderPassComponent* renderPass)
{
	DeleteRenderTargets(renderPass);
	Delete(renderPass->m_ShaderProgram);
	ReleaseFromPoolStatic(m_GPUHandlePools.RenderPasses, m_GPUHandlePools.RenderPassLUT, m_GPUHandlePools.RenderPassPointers, renderPass);
	return true;
}

bool GraphicsResourceService::Delete(ShaderProgramComponent* shaderProgram)
{
	ReleaseFromPoolStatic(m_GPUHandlePools.ShaderPrograms, m_GPUHandlePools.ShaderProgramLUT, m_GPUHandlePools.ShaderProgramPointers, shaderProgram);
	return true;
}

bool GraphicsResourceService::Delete(SamplerComponent* sampler)
{
	ReleaseFromPoolStatic(m_GPUHandlePools.Samplers, m_GPUHandlePools.SamplerLUT, m_GPUHandlePools.SamplerPointers, sampler);
	return true;
}

bool GraphicsResourceService::Delete(GPUBufferComponent* gpuBuffer)
{
	ReleaseFromPoolStatic(m_GPUHandlePools.GPUBuffers, m_GPUHandlePools.GPUBufferLUT, m_GPUHandlePools.GPUBufferPointers, gpuBuffer);
	return true;
}

bool GraphicsResourceService::Delete(CommandListComponent* rhs)
{
	ReleaseFromPoolStatic(m_GPUHandlePools.CommandLists, m_GPUHandlePools.CommandListLUT, m_GPUHandlePools.CommandListPointers, rhs);
	return true;
}

TextureComponent* GraphicsResourceService::FindTextureByName(const char* name)
{
	auto l_result = m_GPUHandlePools.TextureLUT.find(name);
	return (l_result != m_GPUHandlePools.TextureLUT.end()) ? l_result->second : nullptr;
}

MaterialComponent* GraphicsResourceService::FindMaterialByName(const char* name)
{
	auto l_result = m_GPUHandlePools.MaterialLUT.find(name);
	return (l_result != m_GPUHandlePools.MaterialLUT.end()) ? l_result->second : nullptr;
}

bool GraphicsResourceService::WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range)
{
	if (gpuBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_size = gpuBuffer->m_TotalSize;
	if (range != SIZE_MAX)
		l_size = range * gpuBuffer->m_ElementSize;

	if (mappedMemory == nullptr)
	{
		Log(Error, "Can't upload data to GPU buffer: ", gpuBuffer->m_InstanceName, " because it's not mapped.");
		return false;
	}

	std::memcpy((char*)mappedMemory->m_Address + startOffset * gpuBuffer->m_ElementSize, sourceMemory, l_size);
	mappedMemory->m_NeedUploadToGPU = true;

	return true;
}

GPUResourceComponent* GraphicsResourceService::GetTLASBuffer()
{
	return m_TLASBufferComponent;
}

GPUMeshResourceHandle GraphicsResourceService::AllocateMeshResource(const char* name, ObjectLifespan lifespan)
{
	auto l_existing = m_MeshResourceLUT.find(name);
	if (l_existing != m_MeshResourceLUT.end())
		return l_existing->second;

	uint32_t l_index;
	if (!m_FreeMeshResourceSlots.empty())
	{
		l_index = m_FreeMeshResourceSlots.back();
		m_FreeMeshResourceSlots.pop_back();
		m_MeshResources[l_index] = GPUMeshResource();
	}
	else
	{
		l_index = static_cast<uint32_t>(m_MeshResources.size());
		m_MeshResources.emplace_back();
	}

	auto& l_resource = m_MeshResources[l_index];
	l_resource.m_Lifespan = lifespan;
	l_resource.m_Status = ObjectStatus::Created;
	l_resource.m_Name = name;

	GPUMeshResourceHandle l_handle;
	l_handle.m_Index = l_index;
	m_MeshResourceLUT.emplace(name, l_handle);

	return l_handle;
}

void GraphicsResourceService::ReleaseMeshResource(GPUMeshResourceHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return;

	MeshAssetHandle l_assetHandle;
	l_assetHandle.m_Index = handle.m_Index;
	ReleaseMeshGPUResourceImpl(l_assetHandle);

	m_MeshResourceLUT.erase(std::string(l_resource.m_Name.c_str()));
	l_resource = GPUMeshResource();
	m_FreeMeshResourceSlots.push_back(handle.m_Index);
}

void GraphicsResourceService::ReleaseAllMeshResources(ObjectLifespan lifespan)
{
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_MeshResources.size()); i++)
	{
		if (m_MeshResources[i].m_Lifespan == lifespan && m_MeshResources[i].m_Status != ObjectStatus::Invalid)
		{
			GPUMeshResourceHandle l_handle;
			l_handle.m_Index = i;
			ReleaseMeshResource(l_handle);
		}
	}
}

GPUMeshResource* GraphicsResourceService::GetMeshResource(GPUMeshResourceHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return nullptr;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return nullptr;

	return &l_resource;
}

const GPUMeshResource* GraphicsResourceService::GetMeshResource(GPUMeshResourceHandle handle) const
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return nullptr;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return nullptr;

	return &l_resource;
}

GPUMeshResourceHandle GraphicsResourceService::FindMeshResourceByName(const char* name)
{
	auto l_result = m_MeshResourceLUT.find(name);
	if (l_result != m_MeshResourceLUT.end())
		return l_result->second;
	return INVALID_GPU_MESH_HANDLE;
}

void GraphicsResourceService::Initialize(EntityID Entity)
{
	if (m_initializedEntities.count(Entity))
		return;

	m_uninitializedEntities.push(Entity);
	Log(Verbose, "Entity ", Entity, " queued for deferred initialization");
}

void GraphicsResourceService::Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices, EntityID owner)
{
	if (mesh->m_ObjectStatus == ObjectStatus::Activated)
		return;

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_lifespan = (owner != INVALID_ENTITY) ? l_registry->GetLifespan(owner) : ObjectLifespan::Persistence;

	auto l_assetHandle = AssetService::AllocateMeshAsset(mesh->m_InstanceName.c_str(), l_lifespan);
	if (!l_assetHandle.IsValid())
	{
		Log(Error, "Failed to allocate MeshAsset for: ", mesh->m_InstanceName);
		return;
	}

	mesh->m_Asset = l_assetHandle;

	AllocateMeshResource(mesh->m_InstanceName.c_str(), l_lifespan);

	auto* l_resource = AssetService::GetMeshAsset(l_assetHandle);
	if (!vertices.empty())
	{
		l_resource->m_AABB = Math::GenerateAABB(vertices.data(), vertices.size());
		Log(Verbose, "Calculated AABB for MeshComponent: min(",
			l_resource->m_AABB.m_boundMin.x, ",", l_resource->m_AABB.m_boundMin.y, ",", l_resource->m_AABB.m_boundMin.z,
			") max(", l_resource->m_AABB.m_boundMax.x, ",", l_resource->m_AABB.m_boundMax.y, ",", l_resource->m_AABB.m_boundMax.z, ")");
	}

	m_uninitializedMeshes.push(MeshInitTask(mesh, std::move(vertices), std::move(indices), owner, l_lifespan));
	Log(Verbose, "MeshComponent ", mesh->m_InstanceName, " queued for deferred initialization");
}

void GraphicsResourceService::Initialize(TextureComponent* texture, void* textureData, EntityID owner)
{
	if (texture->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_uninitializedTextures.push(TextureInitTask(texture, textureData, owner));
	Log(Verbose, "TextureComponent ", texture->m_InstanceName, " queued for deferred initialization");
}

void GraphicsResourceService::Initialize(MaterialComponent* material, EntityID owner)
{
	if (material->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_uninitializedMaterials.push(MaterialInitTask(material, owner));
	Log(Verbose, "MaterialComponent ", material->m_InstanceName, " queued for deferred initialization");
}

void GraphicsResourceService::Initialize(ShaderProgramComponent* shaderProgram)
{
	InitializeImpl(shaderProgram);
}

void GraphicsResourceService::Initialize(SamplerComponent* sampler)
{
	InitializeImpl(sampler);
}

void GraphicsResourceService::Initialize(GPUBufferComponent* gpuBuffer)
{
	if (gpuBuffer->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_uninitializedGPUBuffers.push(gpuBuffer);
	Log(Verbose, "GPUBufferComponent ", gpuBuffer->m_InstanceName, " queued for deferred initialization");
}

void GraphicsResourceService::Initialize(RenderPassComponent* renderPass)
{
	if (renderPass->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_uninitializedRenderPasses.push(renderPass);
	Log(Verbose, "RenderPassComponent ", renderPass->m_InstanceName, " queued for deferred initialization");
}

void GraphicsResourceService::Initialize(CommandListComponent* commandList)
{
	InitializeImpl(commandList);
}

bool GraphicsResourceService::InitializeComponents()
{
	while (m_uninitializedMeshes.size() > 0)
	{
		MeshInitTask l_task(nullptr, std::vector<Vertex>(), std::vector<Index>());
		m_uninitializedMeshes.tryPop(l_task);

		if (!l_task.m_Component)
			continue;

		MeshComponent* l_meshComp = l_task.m_Component;
		if (l_task.m_Owner != INVALID_ENTITY)
		{
			MeshComponent* l_current = g_Engine->Get<EntityRegistry>()->Get<MeshComponent>(l_task.m_Owner);
			if (l_current)
				l_meshComp = l_current;
			else
				Log(Warning, "MeshInitTask: entity ", l_task.m_Owner, " no longer has MeshComponent, using stored pointer");
		}

		auto* l_resource = AssetService::GetMeshAsset(l_meshComp->m_Asset);
		if (!l_resource)
		{
			Log(Error, "MeshInitTask: invalid MeshAssetHandle for ", l_meshComp->m_InstanceName);
			continue;
		}

		Log(Verbose, "Processing deferred mesh initialization for: ", l_meshComp->m_InstanceName);
		if (InitializeImpl(l_meshComp->m_Asset, l_task.m_Vertices, l_task.m_Indices))
		{
			l_resource->m_Residency = AssetResidency::Resident;
			l_meshComp->m_ObjectStatus = ObjectStatus::Activated;
		}
		else
			m_uninitializedMeshes.push(std::move(l_task));
	}

	while (m_uninitializedTextures.size() > 0)
	{
		TextureInitTask l_task(nullptr, nullptr);
		m_uninitializedTextures.tryPop(l_task);

		if (!l_task.m_Component)
			continue;

		TextureComponent* l_texture = l_task.m_Component;
		if (l_task.m_Owner != INVALID_ENTITY)
		{
			TextureComponent* l_current = g_Engine->Get<EntityRegistry>()->Get<TextureComponent>(l_task.m_Owner);
			if (l_current)
				l_texture = l_current;
			else
				Log(Warning, "TextureInitTask: entity ", l_task.m_Owner, " no longer has TextureComponent, using stored pointer");
		}

		Log(Verbose, "Processing deferred texture initialization for: ", l_texture->m_InstanceName);
		if (InitializeImpl(l_texture, l_task.m_TextureData))
			l_texture->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_uninitializedTextures.push(std::move(l_task));
	}

	while (m_uninitializedMaterials.size() > 0)
	{
		MaterialInitTask l_task(nullptr);
		m_uninitializedMaterials.tryPop(l_task);

		if (!l_task.m_Component)
			continue;

		MaterialComponent* l_material = l_task.m_Component;
		if (l_task.m_Owner != INVALID_ENTITY)
		{
			MaterialComponent* l_current = g_Engine->Get<EntityRegistry>()->Get<MaterialComponent>(l_task.m_Owner);
			if (l_current)
				l_material = l_current;
			else
				Log(Warning, "MaterialInitTask: entity ", l_task.m_Owner, " no longer has MaterialComponent, using stored pointer");
		}

		Log(Verbose, "Processing deferred material initialization for: ", l_material->m_InstanceName);
		if (InitializeImpl(l_material))
			l_material->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_uninitializedMaterials.push(std::move(l_task));
	}

	while (m_uninitializedGPUBuffers.size() > 0)
	{
		GPUBufferComponent* l_component;
		m_uninitializedGPUBuffers.tryPop(l_component);

		if (!l_component)
			continue;

		Log(Verbose, "Processing deferred GPU buffer initialization for: ", l_component->m_InstanceName);
		if (InitializeImpl(l_component))
			l_component->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_uninitializedGPUBuffers.push(std::move(l_component));
	}

	while (m_uninitializedRenderPasses.size() > 0)
	{
		RenderPassComponent* l_component;
		m_uninitializedRenderPasses.tryPop(l_component);

		if (!l_component)
			continue;

		Log(Verbose, "Processing deferred render pass initialization for: ", l_component->m_InstanceName);
		if (InitializeRenderPass(l_component))
			l_component->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_uninitializedRenderPasses.push(std::move(l_component));
	}

	while (m_uninitializedEntities.size() > 0)
	{
		EntityID l_entity;
		m_uninitializedEntities.tryPop(l_entity);

		if (l_entity == INVALID_ENTITY)
			continue;

		Log(Verbose, "Processing deferred entity initialization for: ", l_entity);
		if (InitializeImpl(l_entity))
			m_initializedEntities.emplace(l_entity);
		else
			m_uninitializedEntities.push(std::move(l_entity));
	}

	return true;
}

bool GraphicsResourceService::CreateOutputMergerTargets(RenderPassComponent* renderPass)
{
	if (renderPass->m_RenderPassDesc.m_RenderTargetsCreationFunc)
	{
		Log(Verbose, "Calling customized render targets reservation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_RenderTargetsCreationFunc();
	}
	else
	{
		if (!renderPass->m_OutputMergerTarget)
			Add(renderPass->m_OutputMergerTarget);

		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		l_outputMergerTarget->m_ColorOutputs.resize(renderPass->m_RenderPassDesc.m_RenderTargetCount);
		for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
		{
			auto& l_renderTarget = l_outputMergerTarget->m_ColorOutputs[i];
			l_renderTarget = AddTextureComponent((std::string(renderPass->m_InstanceName.c_str()) + "_RT_" + std::to_string(i) + "/").c_str());
			Log(Verbose, "Render target: ", l_renderTarget->m_InstanceName, " has been allocated at: ", l_renderTarget);
		}
	}

	if (renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc)
	{
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc();
	}
	else if (renderPass->m_RenderPassDesc.m_UseDepthBuffer)
	{
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		auto& l_depthStencilRenderTarget = l_outputMergerTarget->m_DepthStencilOutput;
		l_depthStencilRenderTarget = AddTextureComponent((std::string(renderPass->m_InstanceName.c_str()) + "_DS/").c_str());
		Log(Verbose, renderPass->m_InstanceName.c_str(), " depth stencil target has been allocated.");
	}

	return true;
}

bool GraphicsResourceService::InitializeOutputMergerTargets(RenderPassComponent* renderPass)
{
	if (renderPass->m_RenderPassDesc.m_RenderTargetsInitializationFunc)
	{
		Log(Verbose, "Calling customized render targets creation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_RenderTargetsInitializationFunc();
	}
	else
	{
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
		{
			auto l_renderTarget = l_outputMergerTarget->m_ColorOutputs[i];
			l_renderTarget->m_TextureDesc = renderPass->m_RenderPassDesc.m_RenderTargetDesc;

			InitializeImpl(l_renderTarget, nullptr);
		}

		Log(Verbose, "Render target: ", renderPass->m_InstanceName, " have been created.");
	}

	if (renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsInitializationFunc)
	{
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsInitializationFunc();
	}
	else if (renderPass->m_RenderPassDesc.m_UseDepthBuffer)
	{
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		auto l_depthStencilRenderTarget = l_outputMergerTarget->m_DepthStencilOutput;
		l_depthStencilRenderTarget->m_TextureDesc = renderPass->m_RenderPassDesc.m_RenderTargetDesc;

		if (renderPass->m_RenderPassDesc.m_UseStencilBuffer)
		{
			l_depthStencilRenderTarget->m_TextureDesc.Usage = TextureUsage::DepthStencilAttachment;
			l_depthStencilRenderTarget->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
			l_depthStencilRenderTarget->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
		}
		else
		{
			l_depthStencilRenderTarget->m_TextureDesc.Usage = TextureUsage::DepthAttachment;
			l_depthStencilRenderTarget->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
			l_depthStencilRenderTarget->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
		}

		InitializeImpl(l_depthStencilRenderTarget, nullptr);

		Log(Verbose, renderPass->m_InstanceName, " depth stencil target has been created.");
	}

	return true;
}

bool GraphicsResourceService::InitializeRenderPass(RenderPassComponent* renderPass)
{
	bool l_result = true;

	l_result &= CreateOutputMergerTargets(renderPass);
	l_result &= InitializeOutputMergerTargets(renderPass);
	l_result &= OnOutputMergerTargetsCreated(renderPass);

	renderPass->m_PipelineStateObject = AddPipelineStateObject();
	l_result &= CreatePipelineStateObject(renderPass);

	Log(Verbose, renderPass->m_InstanceName, " PipelineStateObject has been created.");

	renderPass->m_Semaphores.resize(g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount());
	for (size_t i = 0; i < renderPass->m_Semaphores.size(); i++)
	{
		renderPass->m_Semaphores[i] = AddSemaphore();
	}

	Log(Verbose, renderPass->m_InstanceName, " Semaphore has been created.");

	CreateFenceEvents(renderPass);

	return l_result;
}

bool GraphicsResourceService::DeleteRenderTargets(RenderPassComponent* renderPass)
{
	if (renderPass->m_OutputMergerTarget)
	{
		Delete(renderPass->m_OutputMergerTarget);
		renderPass->m_OutputMergerTarget = nullptr;
	}

	if (renderPass->m_PipelineStateObject)
	{
		Delete(renderPass->m_PipelineStateObject);
		renderPass->m_PipelineStateObject = nullptr;
	}

	return true;
}

bool GraphicsResourceService::OnSceneUnloading()
{
	// Drain all GPU queues before releasing resources
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_globalSemaphore = l_fmService->GetGlobalSemaphore();

	l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
	l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Compute);
	l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Copy);

	l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
	l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);
	l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Copy), GPUEngineType::Copy);

	ReleaseAllMeshResources(ObjectLifespan::Scene);
	AssetService::ReleaseAssetsByLifespan(ObjectLifespan::Scene);

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_isSceneBound = [&](EntityID owner) {
		return owner != INVALID_ENTITY
			&& l_registry->GetLifespan(owner) == ObjectLifespan::Scene;
	};

	{
		std::vector<MeshInitTask> l_persistent;
		MeshInitTask l_task(nullptr, {}, {});
		while (m_uninitializedMeshes.tryPop(l_task))
		{
			if (!l_isSceneBound(l_task.m_Owner))
				l_persistent.push_back(std::move(l_task));
		}
		for (auto& t : l_persistent)
			m_uninitializedMeshes.push(std::move(t));
	}

	{
		std::vector<TextureInitTask> l_persistent;
		TextureInitTask l_task(nullptr, nullptr);
		while (m_uninitializedTextures.tryPop(l_task))
		{
			if (!l_isSceneBound(l_task.m_Owner))
				l_persistent.push_back(std::move(l_task));
		}
		for (auto& t : l_persistent)
			m_uninitializedTextures.push(std::move(t));
	}

	{
		std::vector<MaterialInitTask> l_persistent;
		MaterialInitTask l_task(nullptr);
		while (m_uninitializedMaterials.tryPop(l_task))
		{
			if (!l_isSceneBound(l_task.m_Owner))
				l_persistent.push_back(std::move(l_task));
		}
		for (auto& t : l_persistent)
			m_uninitializedMaterials.push(std::move(t));
	}

	EntityID l_entityStale = INVALID_ENTITY;
	while (m_uninitializedEntities.tryPop(l_entityStale)) {}

	OnSceneLoadingStart();
	return true;
}
