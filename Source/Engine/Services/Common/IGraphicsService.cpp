#include "../IGraphicsService.h"

#include "../../Common/Timer.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/TaskScheduler.h"
#include "../../Common/ThreadSafeQueue.h"
#include "../../Common/Randomizer.h"
#include "../../Common/MathHelper.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/AssetService.h"
#include "../../Services/GUIService.h"
#include "../../Services/SceneService.h"
#include "../../Component/TextureComponent.h"
#include "../../Component/MeshComponent.h"
#include "../../Component/MaterialComponent.h"

#include "../../Engine.h"

using namespace Inno;

// @TODO: These should be moved to a separate file.
// Static member definitions
Accessibility Accessibility::Immutable = Accessibility(false, false);
Accessibility Accessibility::ReadOnly = Accessibility(true, false);
Accessibility Accessibility::WriteOnly = Accessibility(false, true);
Accessibility Accessibility::ReadWrite = Accessibility(true, true);
Accessibility Accessibility::CopySource = Accessibility(true, false, true, false);  // read=true, write=false, copySource=true
Accessibility Accessibility::CopyDestination = Accessibility(false, true, false, true);  // read=false, write=true, copyDest=true

bool IGraphicsService::InitializePool()
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

	return true;
}

bool IGraphicsService::TerminatePool()
{
	TObjectPool<MeshComponent>::Destruct(m_GPUHandlePools.Meshes);
	m_GPUHandlePools.Meshes = nullptr;
	TObjectPool<TextureComponent>::Destruct(m_GPUHandlePools.Textures);
	m_GPUHandlePools.Textures = nullptr;
	TObjectPool<MaterialComponent>::Destruct(m_GPUHandlePools.Materials);
	m_GPUHandlePools.Materials = nullptr;
	TObjectPool<RenderPassComponent>::Destruct(m_GPUHandlePools.RenderPasses);
	m_GPUHandlePools.RenderPasses = nullptr;
	TObjectPool<ShaderProgramComponent>::Destruct(m_GPUHandlePools.ShaderPrograms);
	m_GPUHandlePools.ShaderPrograms = nullptr;
	TObjectPool<SamplerComponent>::Destruct(m_GPUHandlePools.Samplers);
	m_GPUHandlePools.Samplers = nullptr;
	TObjectPool<GPUBufferComponent>::Destruct(m_GPUHandlePools.GPUBuffers);
	m_GPUHandlePools.GPUBuffers = nullptr;
	TObjectPool<CommandListComponent>::Destruct(m_GPUHandlePools.CommandLists);
	m_GPUHandlePools.CommandLists = nullptr;
	return true;
}

bool IGraphicsService::Setup(IServiceConfig* systemConfig)
{
	bool l_result = InitializePool();
	if (!l_result)
	{
		Log(Error, "Failed to initialize pool.");
		return false;
	}

	m_swapChainImageCount = 3;
	l_result &= CreateHardwareResources();
	if (!l_result)
	{
		Log(Error, "Failed to create hardware resources.");
		return false;
	}

    m_GlobalGraphicsCommandLists.resize(m_swapChainImageCount);
    for (size_t i = 0; i < m_GlobalGraphicsCommandLists.size(); i++)
    {
        auto l_commandList = AddCommandListComponent(("GlobalGraphicsCommandList_" + std::to_string(i)).c_str());
        Initialize(l_commandList);
        m_GlobalGraphicsCommandLists[i] = l_commandList;
    }

    Log(Success, "Global Graphics CommandLists have been created.");

	m_SwapChainRenderPassComp = AddRenderPassComponent("SwapChain/");
	m_SwapChainShaderProgramComp = AddShaderProgramComponent("SwapChain/");
	m_SwapChainSamplerComp = AddSamplerComponent("SwapChain/");

	m_ObjectStatus = ObjectStatus::Created;

	Log(Success, "GraphicsService Setup finished.");
	return true;
}

bool IGraphicsService::OnSceneUnloading()
{
	// Drain all GPU queues before releasing resources — previous frames may still
	// be in flight referencing scene-scoped mesh/BLAS data.
	SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
	SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Compute);
	SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Copy);

	WaitOnCPU(GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
	WaitOnCPU(GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);
	WaitOnCPU(GetSemaphoreValue(GPUEngineType::Copy), GPUEngineType::Copy);

	ReleaseAllMeshResources(ObjectLifespan::Scene);
	AssetService::ReleaseAssetsByLifespan(ObjectLifespan::Scene);

	// Discard only tasks explicitly owned by a scene-lifespan entity.
	// Tasks with INVALID_ENTITY owner (GPU-pipeline resources) are preserved.
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

bool IGraphicsService::Initialize()
{
	if (m_ObjectStatus != ObjectStatus::Created)
	{
		Log(Error, "GraphicsService is not in Created state.");
		return false;
	}

	m_SwapChainShaderProgramComp->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SwapChainShaderProgramComp->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

	Initialize(m_SwapChainShaderProgramComp);
	Initialize(m_SwapChainSamplerComp);

	InitializeSwapChainRenderPassComponent();

	m_GraphicsSemaphoreValues.resize(m_swapChainImageCount, 0);
	m_ComputeSemaphoreValues.resize(m_swapChainImageCount, 0);
	m_CopySemaphoreValues.resize(m_swapChainImageCount, 0);

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "GraphicsService has been initialized.");

	return true;
}

bool IGraphicsService::InitializeSwapChainRenderPassComponent()
{
	// Skip swap chain render pass initialization in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		Log(Verbose, "InitializeSwapChainRenderPassComponent: Skipping in offscreen mode");
		return true;
	}

	if (!GetSwapChainImages())
	{
		Log(Error, "Failed to get swap chain images.");
		return false;
	}

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&IGraphicsService::AssignSwapChainImages, this);
	l_RenderPassDesc.m_RenderTargetsRemovalFunc = std::bind(&IGraphicsService::ReleaseSwapChainImages, this);

	m_SwapChainRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs.resize(2);

	// t0 - 2D texture (single image from the user pipeline)
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_TextureUsage = TextureUsage::ColorAttachment;

	// s0 - sampler
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Sampler;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;

	m_SwapChainRenderPassComp->m_ShaderProgram = m_SwapChainShaderProgramComp;

	Initialize(m_SwapChainRenderPassComp);

	return true;
}

bool IGraphicsService::Update()
{
	auto l_currentFrame = GetCurrentFrame();

	// Because we are about to reset the current frame's command allocators, we need to make sure that the command lists are not in use.
	WaitOnCPU(m_GraphicsSemaphoreValues[l_currentFrame], GPUEngineType::Graphics);
	WaitOnCPU(m_ComputeSemaphoreValues[l_currentFrame], GPUEngineType::Compute);
	WaitOnCPU(m_CopySemaphoreValues[l_currentFrame], GPUEngineType::Copy);

	BeginFrame();

	InitializeComponents();

	m_UploadHeapPreparationCallback();

	PrepareGlobalCommands();

	ExecuteGlobalCommands();

	if (!g_Engine->Get<SceneService>()->IsLoading())
	{
		m_CommandPreparationCallback();

		PrepareSwapChainCommands();
		g_Engine->Get<GUIService>()->Update();

		// The global commands have to finish before the user pipeline starts.
		WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Graphics);
		WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Compute, GPUEngineType::Graphics);

		m_CommandExecutionCallback();

		// We don't know on which queue the user pipeline output is, so we need to wait for all.
		WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Graphics);
		WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Compute);

		ExecuteSwapChainCommands();
		
		// Only signal on swap chain render pass in windowed mode
		if (!g_Engine->getInitConfig().isOffscreen)
		{
			SignalOnGPU(m_SwapChainRenderPassComp, GPUEngineType::Graphics);
		}

		// The GUI commands must signal on the GPU.
		g_Engine->Get<GUIService>()->ExecuteCommands();
	}

	m_GraphicsSemaphoreValues[l_currentFrame] = GetSemaphoreValue(GPUEngineType::Graphics);
	m_ComputeSemaphoreValues[l_currentFrame] = GetSemaphoreValue(GPUEngineType::Compute);
	m_CopySemaphoreValues[l_currentFrame] = GetSemaphoreValue(GPUEngineType::Copy);

	Present();

	EndFrame();

	m_FrameCountSinceLaunch++;

	return true;
}

bool IGraphicsService::Terminate()
{
	auto l_result = true;
	l_result &= Delete(m_SwapChainSamplerComp);
	l_result &= Delete(m_SwapChainShaderProgramComp);
	l_result &= Delete(m_SwapChainRenderPassComp);

	l_result &= ReleaseHardwareResources();
	l_result &= TerminatePool();

	m_ObjectStatus = ObjectStatus::Terminated;

	if (l_result)
		Log(Success, "GraphicsService has been terminated.");
	else
		Log(Error, "Failed to terminate GraphicsService.");

	return l_result;
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

MeshComponent* IGraphicsService::AddMeshComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.Meshes, m_GPUHandlePools.MeshLUT, m_GPUHandlePools.MeshPointers, name);
}

TextureComponent* IGraphicsService::AddTextureComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.Textures, m_GPUHandlePools.TextureLUT, m_GPUHandlePools.TexturePointers, name);
}

MaterialComponent* IGraphicsService::AddMaterialComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.Materials, m_GPUHandlePools.MaterialLUT, m_GPUHandlePools.MaterialPointers, name);
}

RenderPassComponent* IGraphicsService::AddRenderPassComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.RenderPasses, m_GPUHandlePools.RenderPassLUT, m_GPUHandlePools.RenderPassPointers, name);
}

ShaderProgramComponent* IGraphicsService::AddShaderProgramComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.ShaderPrograms, m_GPUHandlePools.ShaderProgramLUT, m_GPUHandlePools.ShaderProgramPointers, name);
}

SamplerComponent* IGraphicsService::AddSamplerComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.Samplers, m_GPUHandlePools.SamplerLUT, m_GPUHandlePools.SamplerPointers, name);
}

GPUBufferComponent* IGraphicsService::AddGPUBufferComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.GPUBuffers, m_GPUHandlePools.GPUBufferLUT, m_GPUHandlePools.GPUBufferPointers, name);
}

CommandListComponent* IGraphicsService::AddCommandListComponent(const char* name)
{
	return AllocateGPUHandle(m_GPUHandlePools.CommandLists, m_GPUHandlePools.CommandListLUT, m_GPUHandlePools.CommandListPointers, name);
}

GPUMeshResourceHandle IGraphicsService::AllocateMeshResource(const char* name, ObjectLifespan lifespan)
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

void IGraphicsService::ReleaseMeshResource(GPUMeshResourceHandle handle)
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

void IGraphicsService::ReleaseAllMeshResources(ObjectLifespan lifespan)
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

GPUMeshResource* IGraphicsService::GetMeshResource(GPUMeshResourceHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return nullptr;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return nullptr;

	return &l_resource;
}

const GPUMeshResource* IGraphicsService::GetMeshResource(GPUMeshResourceHandle handle) const
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return nullptr;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return nullptr;

	return &l_resource;
}

GPUMeshResourceHandle IGraphicsService::FindMeshResourceByName(const char* name)
{
	auto l_result = m_MeshResourceLUT.find(name);
	if (l_result != m_MeshResourceLUT.end())
		return l_result->second;
	return INVALID_GPU_MESH_HANDLE;
}

TextureComponent* IGraphicsService::FindTextureByName(const char* name)
{
	auto l_result = m_GPUHandlePools.TextureLUT.find(name);
	return (l_result != m_GPUHandlePools.TextureLUT.end()) ? l_result->second : nullptr;
}

MaterialComponent* IGraphicsService::FindMaterialByName(const char* name)
{
	auto l_result = m_GPUHandlePools.MaterialLUT.find(name);
	return (l_result != m_GPUHandlePools.MaterialLUT.end()) ? l_result->second : nullptr;
}

void IGraphicsService::Initialize(EntityID Entity)
{
	if (m_initializedEntities.count(Entity))
		return;

	m_uninitializedEntities.push(Entity);
	Log(Verbose, "Entity ", Entity, " queued for deferred initialization");
}

void IGraphicsService::Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices, EntityID owner)
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

void IGraphicsService::Initialize(TextureComponent* texture, void* textureData, EntityID owner)
{
	if (texture->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_uninitializedTextures.push(TextureInitTask(texture, textureData, owner));
	Log(Verbose, "TextureComponent ", texture->m_InstanceName, " queued for deferred initialization");
}

void IGraphicsService::Initialize(MaterialComponent* material, EntityID owner)
{
	if (material->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_uninitializedMaterials.push(MaterialInitTask(material, owner));
	Log(Verbose, "MaterialComponent ", material->m_InstanceName, " queued for deferred initialization");
}

void IGraphicsService::Initialize(ShaderProgramComponent* shaderProgram)
{
	InitializeImpl(shaderProgram);
}

void IGraphicsService::Initialize(SamplerComponent* sampler)
{
	InitializeImpl(sampler);
}

void IGraphicsService::Initialize(GPUBufferComponent* gpuBuffer)
{
	if (gpuBuffer->m_ObjectStatus == ObjectStatus::Activated)
		return;

	// Queue GPU buffer for deferred initialization
	m_uninitializedGPUBuffers.push(gpuBuffer);
	Log(Verbose, "GPUBufferComponent ", gpuBuffer->m_InstanceName, " queued for deferred initialization");
}

void IGraphicsService::Initialize(RenderPassComponent* renderPass)
{
	if (renderPass->m_ObjectStatus == ObjectStatus::Activated)
		return;

	// Queue render pass for deferred initialization
	m_uninitializedRenderPasses.push(renderPass);
	Log(Verbose, "RenderPassComponent ", renderPass->m_InstanceName, " queued for deferred initialization");
}

void IGraphicsService::Initialize(CommandListComponent* commandList)
{
	InitializeImpl(commandList);
}

bool IGraphicsService::CreateOutputMergerTargets(RenderPassComponent* renderPass)
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
		auto l_swapChainImageCount = GetSwapChainImageCount();
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

bool IGraphicsService::InitializeOutputMergerTargets(RenderPassComponent* renderPass)
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

bool IGraphicsService::SignalOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType)
{
	if (renderPass == nullptr)
	{
		return SignalOnGPU(static_cast<ISemaphore*>(nullptr), queueType);
	}

	// Add defensive checks for corrupted RenderPassComponent
	if (renderPass->m_CurrentFrame >= renderPass->m_Semaphores.size())
	{
		Log(Error, "SignalOnGPU: Invalid m_CurrentFrame index %d for RenderPass %s (semaphore count: %d)", 
			renderPass->m_CurrentFrame, renderPass->m_InstanceName.c_str(), renderPass->m_Semaphores.size());
		return false;
	}

	auto l_semaphore = renderPass->m_Semaphores[renderPass->m_CurrentFrame];
	return SignalOnGPU(l_semaphore, queueType);
}

bool IGraphicsService::WaitOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	if (renderPass == nullptr)
	{
		return WaitOnGPU(static_cast<ISemaphore*>(nullptr), queueType, semaphoreType);
	}

	// Add defensive checks for corrupted RenderPassComponent
	if (renderPass->m_CurrentFrame >= renderPass->m_Semaphores.size())
	{
		Log(Error, "WaitOnGPU: Invalid m_CurrentFrame index %d for RenderPass %s (semaphore count: %d)", 
			renderPass->m_CurrentFrame, renderPass->m_InstanceName.c_str(), renderPass->m_Semaphores.size());
		return false;
	}

	auto l_semaphore = renderPass->m_Semaphores[renderPass->m_CurrentFrame];
	return WaitOnGPU(l_semaphore, queueType, semaphoreType);
}

bool IGraphicsService::CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex)
{
	if (!commandList || !renderPass)
		return false;

	return Open(commandList, commandList->m_Type, renderPass->m_PipelineStateObject);
}

bool IGraphicsService::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in CommandListEnd");
		return false;
	}

	if (!Close(commandList, renderPass->m_RenderPassDesc.m_GPUEngineType))
	{
		Log(Error, "Failed to close command list for render pass ", renderPass->m_InstanceName);
		return false;
	}
	
	return true;
}

bool IGraphicsService::ChangeRenderTargetStates(RenderPassComponent* renderPass, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;

	for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
	{
		auto l_renderTarget = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_ColorOutputs[i]);
		TryToTransitState(l_renderTarget, commandList, sourceAccessibility, targetAccessibility);
	}

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
	{
		auto l_depthStencilRenderTarget = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_DepthStencilOutput);
		TryToTransitState(l_depthStencilRenderTarget, commandList, sourceAccessibility, targetAccessibility);
	}

	return true;
}

bool IGraphicsService::Present()
{
	PresentImpl();

	if (m_needResize)
	{
		// The present operation might be an asynchronous operation, so we need to make sure that the GPU has finished the present operation.
		SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
		SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Compute);
		SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Copy);

		auto l_graphicsSemaphoreValue = GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = GetSemaphoreValue(GPUEngineType::Compute);
		auto l_copySemaphoreValue = GetSemaphoreValue(GPUEngineType::Copy);

		WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);
		WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		WaitOnCPU(l_copySemaphoreValue, GPUEngineType::Copy);

		ExecuteResize();

		m_needResize = false;
	}

	return true;
}

bool IGraphicsService::SetUserPipelineOutput(std::function<GPUResourceComponent* ()>&& getUserPipelineOutputFunc)
{
	m_GetUserPipelineOutputFunc = getUserPipelineOutputFunc;
	return true;
}

GPUResourceComponent* IGraphicsService::GetUserPipelineOutput()
{
	return m_GetUserPipelineOutputFunc();
}

bool IGraphicsService::Resize()
{
	m_needResize = true;
	return true;
}

bool IGraphicsService::WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range)
{
	if (gpuBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_size = gpuBuffer->m_TotalSize;
	if (range != SIZE_MAX)
		l_size = range * gpuBuffer->m_ElementSize;

	auto l_currentFrame = GetCurrentFrame();
	if (mappedMemory == nullptr)
	{
		if (gpuBuffer->m_ObjectStatus == ObjectStatus::Activated)
		{
			Log(Error, "Can't upload data to GPU buffer: ", gpuBuffer->m_InstanceName, " because it's not mapped.");
		}
		return false;
	}

	std::memcpy((char*)mappedMemory->m_Address + startOffset * gpuBuffer->m_ElementSize, sourceMemory, l_size);

	mappedMemory->m_NeedUploadToGPU = true;

	return true;
}

bool IGraphicsService::InitializeImpl(MaterialComponent* material)
{
	material->m_GPUResourceType = GPUResourceType::Material;

	return true;
}

bool IGraphicsService::InitializeImpl(RenderPassComponent* renderPass)
{
	bool l_result = true;

	l_result &= CreateOutputMergerTargets(renderPass);

	l_result &= InitializeOutputMergerTargets(renderPass);

	l_result &= OnOutputMergerTargetsCreated(renderPass);

	renderPass->m_PipelineStateObject = AddPipelineStateObject();

	l_result &= CreatePipelineStateObject(renderPass);

	// Command lists are now created dynamically during execution
	Log(Verbose, renderPass->m_InstanceName, " PipelineStateObject has been created.");

	renderPass->m_Semaphores.resize(GetSwapChainImageCount());
	for (size_t i = 0; i < renderPass->m_Semaphores.size(); i++)
	{
		renderPass->m_Semaphores[i] = AddSemaphore();
	}

	Log(Verbose, renderPass->m_InstanceName, " Semaphore has been created.");

	CreateFenceEvents(renderPass);

	renderPass->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

bool IGraphicsService::DeleteRenderTargets(RenderPassComponent* renderPass)
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

void IGraphicsService::SetUploadHeapPreparationCallback(std::function<bool()>&& callback)
{
	m_UploadHeapPreparationCallback = callback;
}

void IGraphicsService::SetCommandPreparationCallback(std::function<bool()>&& callback)
{
	m_CommandPreparationCallback = callback;
}

void IGraphicsService::SetCommandExecutionCallback(std::function<bool()>&& callback)
{
	m_CommandExecutionCallback = callback;
}

uint32_t IGraphicsService::GetSwapChainImageCount()
{
	return m_swapChainImageCount;
}

RenderPassComponent* IGraphicsService::GetSwapChainRenderPassComponent()
{
	return m_SwapChainRenderPassComp;
}

uint32_t IGraphicsService::GetPreviousFrame()
{
	auto l_previousFrame = m_CurrentFrame == 0 ? m_swapChainImageCount - 1 : m_CurrentFrame - 1;
	return l_previousFrame;
}

uint32_t IGraphicsService::GetCurrentFrame()
{
	return m_CurrentFrame;
}

uint32_t IGraphicsService::GetNextFrame()
{
	auto l_nextFrame = m_CurrentFrame == m_swapChainImageCount - 1 ? 0 : m_CurrentFrame + 1;
	return l_nextFrame;
}

uint32_t IGraphicsService::GetFrameCountSinceLaunch()
{
	return m_FrameCountSinceLaunch;
}

bool IGraphicsService::InitializeComponents()
{
	// Process queued mesh initialization tasks
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

	// Process queued texture initialization tasks
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

	// Process queued material components
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

	// Process queued GPU buffer components
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

	// Process queued render pass components
	while (m_uninitializedRenderPasses.size() > 0)
	{
		RenderPassComponent* l_component;
		m_uninitializedRenderPasses.tryPop(l_component);

		if (!l_component)
			continue;

		Log(Verbose, "Processing deferred render pass initialization for: ", l_component->m_InstanceName);
		if (InitializeImpl(l_component))
			l_component->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_uninitializedRenderPasses.push(std::move(l_component));
	}

	// Process queued entity initializations
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

bool IGraphicsService::PrepareGlobalCommands()
{
	auto l_currentFrame = GetCurrentFrame();

	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	Open(l_commandList, GPUEngineType::Graphics);

	// for (auto i : m_initializedTextures)
	// {
	// 	if (i->m_MappedMemories.size() == 0)
	// 		continue;

	// 	auto l_mappedMemoryIndex = i->m_TextureDesc.IsMultiBuffer ? l_currentFrame : 0;
	// 	auto l_mappedMemory = i->m_MappedMemories[l_mappedMemoryIndex];
	// 	if (l_mappedMemory->m_NeedUploadToGPU)
	// 	{
	// 		UploadToGPU(l_commandList, i);
	// 		l_mappedMemory->m_NeedUploadToGPU = false;
	// 	}
	// }

	for (auto i : m_GPUHandlePools.GPUBufferPointers)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			continue;
		if (i->m_MappedMemories.size() == 0)
			continue;

		auto l_mappedMemory = i->m_MappedMemories[l_currentFrame];
		if (l_mappedMemory->m_NeedUploadToGPU)
		{
			TryToTransitState(i, l_commandList, Accessibility::ReadOnly, Accessibility::CopyDestination);
			UploadToGPU(l_commandList, i);
			TryToTransitState(i, l_commandList, Accessibility::CopyDestination, Accessibility::ReadOnly);
			l_mappedMemory->m_NeedUploadToGPU = false;
		}
	}

	PrepareRayTracing(l_commandList);

	Close(l_commandList, GPUEngineType::Graphics);

	return true;
}

bool IGraphicsService::ExecuteGlobalCommands()
{
	auto l_currentFrame = GetCurrentFrame();

	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	Execute(l_commandList, GPUEngineType::Graphics);
	SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);

	return true;
}

bool IGraphicsService::PrepareSwapChainCommands()
{
	// Skip swap chain commands in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		//Log(Verbose, "IGraphicsService: Skipping swap chain commands in offscreen mode");
		return true;
	}

	auto l_userPipelineOutput = m_GetUserPipelineOutputFunc();
	if (!l_userPipelineOutput)
		return false;

	if (l_userPipelineOutput->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	// Get the command list for swap chain rendering
	auto l_currentFrame = GetCurrentFrame();
	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	
	CommandListBegin(m_SwapChainRenderPassComp, l_commandList, l_currentFrame);

	// User pipeline output was written to, now we need to read from it
	TryToTransitState(reinterpret_cast<TextureComponent*>(l_userPipelineOutput), l_commandList, Accessibility::WriteOnly, Accessibility::ReadOnly);
	BindRenderPassComponent(m_SwapChainRenderPassComp, l_commandList);

	ClearRenderTargets(m_SwapChainRenderPassComp, l_commandList);

	BindGPUResource(m_SwapChainRenderPassComp, l_commandList, ShaderStage::Pixel, l_userPipelineOutput, 0);
	BindGPUResource(m_SwapChainRenderPassComp, l_commandList, ShaderStage::Pixel, m_SwapChainSamplerComp, 1);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(MeshShape::Square);

	DrawIndexedInstanced(m_SwapChainRenderPassComp, l_commandList, l_mesh, 1);

	TryToTransitState(m_SwapChainRenderPassComp->m_OutputMergerTarget->m_ColorOutputs[0], l_commandList, Accessibility::WriteOnly, Accessibility::ReadOnly);
	
	CommandListEnd(m_SwapChainRenderPassComp, l_commandList);

	return true;
}

bool IGraphicsService::ExecuteSwapChainCommands()
{
	// Skip swap chain execution in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		//Log(Verbose, "IGraphicsService: Skipping swap chain execution in offscreen mode");
		return true;
	}

	auto l_userPipelineOutput = m_GetUserPipelineOutputFunc();
	if (!l_userPipelineOutput)
		return false;

	if (l_userPipelineOutput->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	// Execute the swap chain command list
	auto l_currentFrame = GetCurrentFrame();
	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	Execute(l_commandList, GPUEngineType::Graphics);

	return true;
}

bool IGraphicsService::ExecuteResize()
{
	PreResize();
	ResizeImpl();
	PostResize();

	return true;
}

bool IGraphicsService::PreResize()
{
	for (auto i : m_GPUHandlePools.RenderPassPointers)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			continue;
		if (!PreResize(i))
		{
			Log(Error, "Can't delete resources for ", i->m_InstanceName, " when resizing.");
			return false;
		}
	}

	return true;
}

bool IGraphicsService::PreResize(RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	DeleteRenderTargets(renderPass);

	return true;
}

bool IGraphicsService::PostResize()
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	for (auto i : m_GPUHandlePools.RenderPassPointers)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			continue;
		if (!PostResize(l_screenResolution, i))
		{
			Log(Error, "Can't resize ", i->m_InstanceName);
			return false;
		}
	}

	return true;
}

bool IGraphicsService::PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	renderPass->m_RenderPassDesc.m_RenderTargetDesc.Width = screenResolution.x;
	renderPass->m_RenderPassDesc.m_RenderTargetDesc.Height = screenResolution.y;

	renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)screenResolution.x;
	renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)screenResolution.y;

	CreateOutputMergerTargets(renderPass);
	InitializeOutputMergerTargets(renderPass);

	OnOutputMergerTargetsCreated(renderPass);

	renderPass->m_PipelineStateObject = AddPipelineStateObject();

	CreatePipelineStateObject(renderPass);

	if (renderPass->m_OnResize)
		renderPass->m_OnResize();

	return true;
}