#include "../IRenderingServer.h"

#include "../../Common/Timer.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/TaskScheduler.h"
#include "../../Common/ThreadSafeQueue.h"
#include "../../Common/Randomizer.h"
#include "../../Common/MathHelper.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/GUISystem.h"
#include "../../Services/SceneService.h"
#include "../../Services/EntityManager.h"
#include "../../Services/ComponentManager.h"

#include "../../Engine.h"
#include "../IRenderingServer.h"

using namespace Inno;

// @TODO: These should be moved to a separate file.
// Static member definitions
Accessibility Accessibility::Immutable = Accessibility(false, false);
Accessibility Accessibility::ReadOnly = Accessibility(true, false);
Accessibility Accessibility::WriteOnly = Accessibility(false, true);
Accessibility Accessibility::ReadWrite = Accessibility(true, true);
Accessibility Accessibility::CopySource = Accessibility(true, false, true, false);  // read=true, write=false, copySource=true
Accessibility Accessibility::CopyDestination = Accessibility(false, true, false, true);  // read=false, write=true, copyDest=true

bool IRenderingServer::InitializePool()
{
	auto l_renderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	g_Engine->Get<ComponentManager>()->RegisterType<MeshComponent>(l_renderingCapability.maxMeshes, this);
	g_Engine->Get<ComponentManager>()->RegisterType<TextureComponent>(l_renderingCapability.maxTextures, this);
	g_Engine->Get<ComponentManager>()->RegisterType<MaterialComponent>(l_renderingCapability.maxMaterials, this);
	g_Engine->Get<ComponentManager>()->RegisterType<RenderPassComponent>(128, this);
	g_Engine->Get<ComponentManager>()->RegisterType<ShaderProgramComponent>(256, this);
	g_Engine->Get<ComponentManager>()->RegisterType<SamplerComponent>(256, this);
	g_Engine->Get<ComponentManager>()->RegisterType<GPUBufferComponent>(l_renderingCapability.maxBuffers, this);
	g_Engine->Get<ComponentManager>()->RegisterType<CommandListComponent>(256, this);

	return true;
}

bool IRenderingServer::TerminatePool()
{
	return true;
}

bool IRenderingServer::Setup(ISystemConfig* systemConfig)
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

	m_SceneLoadingStartedCallback = [this]() { OnSceneLoadingStart(); };

	g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&m_SceneLoadingStartedCallback, 0);

	Log(Success, "RenderingServer Setup finished.");
	return true;
}

bool IRenderingServer::Initialize()
{
	if (m_ObjectStatus != ObjectStatus::Created)
	{
		Log(Error, "RenderingServer is not in Created state.");
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
	Log(Success, "RenderingServer has been initialized.");

	return true;
}

bool IRenderingServer::InitializeSwapChainRenderPassComponent()
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
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&IRenderingServer::AssignSwapChainImages, this);
	l_RenderPassDesc.m_RenderTargetsRemovalFunc = std::bind(&IRenderingServer::ReleaseSwapChainImages, this);

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

bool IRenderingServer::Update()
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
		g_Engine->Get<GUISystem>()->Update();

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
		g_Engine->Get<GUISystem>()->ExecuteCommands();
	}

	m_GraphicsSemaphoreValues[l_currentFrame] = GetSemaphoreValue(GPUEngineType::Graphics);
	m_ComputeSemaphoreValues[l_currentFrame] = GetSemaphoreValue(GPUEngineType::Compute);
	m_CopySemaphoreValues[l_currentFrame] = GetSemaphoreValue(GPUEngineType::Copy);

	Present();

	EndFrame();

	m_FrameCountSinceLaunch++;

	return true;
}

bool IRenderingServer::Terminate()
{
	auto l_result = true;
	l_result &= Delete(m_SwapChainSamplerComp);
	l_result &= Delete(m_SwapChainShaderProgramComp);
	l_result &= Delete(m_SwapChainRenderPassComp);

	l_result &= ReleaseHardwareResources();
	l_result &= TerminatePool();

	m_ObjectStatus = ObjectStatus::Terminated;

	if (l_result)
		Log(Success, "RenderingServer has been terminated.");
	else
		Log(Error, "Failed to terminate RenderingServer.");

	return l_result;
}

template <typename T>
T* AddComponent(const char* name)
{
	static std::atomic<uint32_t> l_count = 0;
	l_count++;
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = (std::string(T::GetTypeName()) + "_" + std::to_string(l_count) + "/");
	}

	if (strcmp(name, "") == 0)
	{
		Log(Error, "Component name cannot be empty.");
		return nullptr;
	}

	auto l_parentEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Persistence, l_name.c_str());
	auto l_component = g_Engine->Get<ComponentManager>()->Spawn<T>(l_parentEntity, false, ObjectLifespan::Persistence);
	if (!l_component)
	{
		Log(Error, "Failed to allocate component from the pool.");
		return nullptr;
	}

	return l_component;
}

MeshComponent* IRenderingServer::AddMeshComponent(const char* name)
{
	return AddComponent<MeshComponent>(name);
}

TextureComponent* IRenderingServer::AddTextureComponent(const char* name)
{
	return AddComponent<TextureComponent>(name);
}

MaterialComponent* IRenderingServer::AddMaterialComponent(const char* name)
{
	return AddComponent<MaterialComponent>(name);
}

RenderPassComponent* IRenderingServer::AddRenderPassComponent(const char* name)
{
	return AddComponent<RenderPassComponent>(name);
}

ShaderProgramComponent* IRenderingServer::AddShaderProgramComponent(const char* name)
{
	return AddComponent<ShaderProgramComponent>(name);
}

SamplerComponent* IRenderingServer::AddSamplerComponent(const char* name)
{
	return AddComponent<SamplerComponent>(name);
}

GPUBufferComponent* IRenderingServer::AddGPUBufferComponent(const char* name)
{
	return AddComponent<GPUBufferComponent>(name);
}

CommandListComponent* IRenderingServer::AddCommandListComponent(const char* name)
{
	return AddComponent<CommandListComponent>(name);
}

void IRenderingServer::Initialize(ModelComponent* model)
{
	if (std::find(m_initializedModels.begin(), m_initializedModels.end(), model) != m_initializedModels.end())
		return;

	// Queue model for deferred initialization
	m_uninitializedModels.push(model);
	Log(Verbose, "ModelComponent ", model->m_InstanceName, " queued for deferred initialization");
}

void IRenderingServer::Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	if (std::find(m_initializedMeshes.begin(), m_initializedMeshes.end(), mesh) != m_initializedMeshes.end())
		return;

	// Calculate AABB from vertex data before queuing for deferred initialization
	if (!vertices.empty())
	{
		mesh->m_AABB = Math::GenerateAABB(vertices.data(), vertices.size());
		Log(Verbose, "Calculated AABB for MeshComponent: min(",
			mesh->m_AABB.m_boundMin.x, ",", mesh->m_AABB.m_boundMin.y, ",", mesh->m_AABB.m_boundMin.z,
			") max(", mesh->m_AABB.m_boundMax.x, ",", mesh->m_AABB.m_boundMax.y, ",", mesh->m_AABB.m_boundMax.z, ")");
	}

	// Queue mesh for deferred initialization using move semantics to avoid copying vertex/index data
	m_uninitializedMeshes.push(MeshInitTask(mesh, std::move(vertices), std::move(indices)));
	Log(Verbose, "MeshComponent ", mesh->m_InstanceName, " queued for deferred initialization");
}

void IRenderingServer::Initialize(TextureComponent* texture, void* textureData)
{
	if (std::find(m_initializedTextures.begin(), m_initializedTextures.end(), texture) != m_initializedTextures.end())
		return;

	// Queue texture for deferred initialization
	m_uninitializedTextures.push(TextureInitTask(texture, textureData));
	Log(Verbose, "TextureComponent ", texture->m_InstanceName, " queued for deferred initialization");
}

void IRenderingServer::Initialize(MaterialComponent* material)
{
	if (std::find(m_initializedMaterials.begin(), m_initializedMaterials.end(), material) != m_initializedMaterials.end())
		return;

	// Queue material for deferred initialization
	m_uninitializedMaterials.push(material);
	Log(Verbose, "MaterialComponent ", material->m_InstanceName, " queued for deferred initialization");
}

void IRenderingServer::Initialize(ShaderProgramComponent* shaderProgram)
{
	InitializeImpl(shaderProgram);
}

void IRenderingServer::Initialize(SamplerComponent* sampler)
{
	InitializeImpl(sampler);
}

void IRenderingServer::Initialize(GPUBufferComponent* gpuBuffer)
{
	if (std::find(m_initializedGPUBuffers.begin(), m_initializedGPUBuffers.end(), gpuBuffer) != m_initializedGPUBuffers.end())
		return;

	// Queue GPU buffer for deferred initialization
	m_uninitializedGPUBuffers.push(gpuBuffer);
	Log(Verbose, "GPUBufferComponent ", gpuBuffer->m_InstanceName, " queued for deferred initialization");
}

void IRenderingServer::Initialize(RenderPassComponent* renderPass)
{
	if (std::find(m_initializedRenderPasses.begin(), m_initializedRenderPasses.end(), renderPass) != m_initializedRenderPasses.end())
		return;

	// Queue render pass for deferred initialization
	m_uninitializedRenderPasses.push(renderPass);
	Log(Verbose, "RenderPassComponent ", renderPass->m_InstanceName, " queued for deferred initialization");
}

void IRenderingServer::Initialize(CommandListComponent* commandList)
{
	InitializeImpl(commandList);
}

bool IRenderingServer::CreateOutputMergerTargets(RenderPassComponent* renderPass)
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

bool IRenderingServer::InitializeOutputMergerTargets(RenderPassComponent* renderPass)
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

bool IRenderingServer::SignalOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType)
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

bool IRenderingServer::WaitOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType, GPUEngineType semaphoreType)
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

bool IRenderingServer::CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex)
{
	if (!commandList || !renderPass)
		return false;

	return Open(commandList, commandList->m_Type, renderPass->m_PipelineStateObject);
}

bool IRenderingServer::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
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

bool IRenderingServer::ChangeRenderTargetStates(RenderPassComponent* renderPass, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
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

bool IRenderingServer::Present()
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

bool IRenderingServer::SetUserPipelineOutput(std::function<GPUResourceComponent* ()>&& getUserPipelineOutputFunc)
{
	m_GetUserPipelineOutputFunc = getUserPipelineOutputFunc;
	return true;
}

GPUResourceComponent* IRenderingServer::GetUserPipelineOutput()
{
	return m_GetUserPipelineOutputFunc();
}

bool IRenderingServer::Resize()
{
	m_needResize = true;
	return true;
}

bool IRenderingServer::WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range)
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

bool IRenderingServer::InitializeImpl(MaterialComponent* material)
{
	material->m_GPUResourceType = GPUResourceType::Material;
	material->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool IRenderingServer::InitializeImpl(RenderPassComponent* renderPass)
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

bool IRenderingServer::DeleteRenderTargets(RenderPassComponent* renderPass)
{
	Delete(renderPass->m_OutputMergerTarget);
	renderPass->m_OutputMergerTarget = nullptr;

	Delete(renderPass->m_PipelineStateObject);

	return true;
}

void IRenderingServer::SetUploadHeapPreparationCallback(std::function<bool()>&& callback)
{
	m_UploadHeapPreparationCallback = callback;
}

void IRenderingServer::SetCommandPreparationCallback(std::function<bool()>&& callback)
{
	m_CommandPreparationCallback = callback;
}

void IRenderingServer::SetCommandExecutionCallback(std::function<bool()>&& callback)
{
	m_CommandExecutionCallback = callback;
}

uint32_t IRenderingServer::GetSwapChainImageCount()
{
	return m_swapChainImageCount;
}

RenderPassComponent* IRenderingServer::GetSwapChainRenderPassComponent()
{
	return m_SwapChainRenderPassComp;
}

uint32_t IRenderingServer::GetPreviousFrame()
{
	auto l_previousFrame = m_CurrentFrame == 0 ? m_swapChainImageCount - 1 : m_CurrentFrame - 1;
	return l_previousFrame;
}

uint32_t IRenderingServer::GetCurrentFrame()
{
	return m_CurrentFrame;
}

uint32_t IRenderingServer::GetNextFrame()
{
	auto l_nextFrame = m_CurrentFrame == m_swapChainImageCount - 1 ? 0 : m_CurrentFrame + 1;
	return l_nextFrame;
}

uint32_t IRenderingServer::GetFrameCountSinceLaunch()
{
	return m_FrameCountSinceLaunch;
}

bool IRenderingServer::InitializeComponents()
{
	// Process queued mesh initialization tasks
	while (m_uninitializedMeshes.size() > 0)
	{
		MeshInitTask l_task(nullptr, std::vector<Vertex>(), std::vector<Index>());
		m_uninitializedMeshes.tryPop(l_task);

		if (!l_task.m_Component)
			continue;

		Log(Verbose, "Processing deferred mesh initialization for: ", l_task.m_Component->m_InstanceName);
		if (InitializeImpl(l_task.m_Component, l_task.m_Vertices, l_task.m_Indices))
			m_initializedMeshes.emplace(l_task.m_Component);
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

		Log(Verbose, "Processing deferred texture initialization for: ", l_task.m_Component->m_InstanceName);
		if (InitializeImpl(l_task.m_Component, l_task.m_TextureData))
			m_initializedTextures.emplace(l_task.m_Component);
		else
			m_uninitializedTextures.push(std::move(l_task));
	}

	// Process queued material components
	while (m_uninitializedMaterials.size() > 0)
	{
		MaterialComponent* l_component;
		m_uninitializedMaterials.tryPop(l_component);

		if (!l_component)
			continue;

		Log(Verbose, "Processing deferred material initialization for: ", l_component->m_InstanceName);
		if (InitializeImpl(l_component))
			m_initializedMaterials.emplace(l_component);
		else
			m_uninitializedMaterials.push(std::move(l_component));
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
			m_initializedGPUBuffers.emplace(l_component);
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
			m_initializedRenderPasses.emplace_back(l_component);
		else
			m_uninitializedRenderPasses.push(std::move(l_component));
	}

	// Process queued model components
	while (m_uninitializedModels.size() > 0)
	{
		ModelComponent* l_component;
		m_uninitializedModels.tryPop(l_component);

		if (!l_component)
			continue;

		Log(Verbose, "Processing deferred model initialization for: ", l_component->m_InstanceName);
		if (InitializeImpl(l_component))
			m_initializedModels.emplace(l_component);
		else
			m_uninitializedModels.push(std::move(l_component));
	}

	return true;
}

bool IRenderingServer::PrepareGlobalCommands()
{
	auto l_currentFrame = GetCurrentFrame();

	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	Open(l_commandList, GPUEngineType::Graphics);

	// for (auto i : m_initializedMeshes)
	// {
	// 	if (i->m_NeedUploadToGPU)
	// 	{
	// 		UploadToGPU(l_commandList, i);
	// 		i->m_NeedUploadToGPU = false;
	// 	}
	// }

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

	for (auto i : m_initializedGPUBuffers)
	{
		if (i->m_MappedMemories.size() == 0)
			continue;

		auto l_mappedMemory = i->m_MappedMemories[l_currentFrame];
		if (l_mappedMemory->m_NeedUploadToGPU)
		{
			// Transition to copy destination, upload, then transition back
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

bool IRenderingServer::ExecuteGlobalCommands()
{
	auto l_currentFrame = GetCurrentFrame();

	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	Execute(l_commandList, GPUEngineType::Graphics);
	SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);

	return true;
}

bool IRenderingServer::PrepareSwapChainCommands()
{
	// Skip swap chain commands in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		//Log(Verbose, "IRenderingServer: Skipping swap chain commands in offscreen mode");
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

bool IRenderingServer::ExecuteSwapChainCommands()
{
	// Skip swap chain execution in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		//Log(Verbose, "IRenderingServer: Skipping swap chain execution in offscreen mode");
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

bool IRenderingServer::ExecuteResize()
{
	PreResize();
	ResizeImpl();
	PostResize();

	return true;
}

bool IRenderingServer::PreResize()
{
	for (auto i : m_initializedRenderPasses)
	{
		if (!PreResize(i))
		{
			Log(Error, "Can't delete resources for ", i->m_InstanceName, " when resizing.");
			return false;
		}
	}

	return true;
}

bool IRenderingServer::PreResize(RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	DeleteRenderTargets(renderPass);

	return true;
}

bool IRenderingServer::PostResize()
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	for (auto i : m_initializedRenderPasses)
	{
		if (!PostResize(l_screenResolution, i))
		{
			Log(Error, "Can't resize ", i->m_InstanceName);
			return false;
		}
	}

	return true;
}

bool IRenderingServer::PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* renderPass)
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