#include "DX12RenderingServer.h"

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Common/Randomizer.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/EntityManager.h"

#include "../../Engine.h"

using namespace Inno;

bool DX12RenderingServer::InitializePool()
{
	auto l_renderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	m_MeshComponentPool = TObjectPool<DX12MeshComponent>::Create(l_renderingCapability.maxMeshes);
	m_TextureComponentPool = TObjectPool<DX12TextureComponent>::Create(l_renderingCapability.maxTextures);
	m_MaterialComponentPool = TObjectPool<DX12MaterialComponent>::Create(l_renderingCapability.maxMaterials);
	m_RenderPassComponentPool = TObjectPool<RenderPassComponent>::Create(128);
	m_PSOPool = TObjectPool<DX12PipelineStateObject>::Create(128);
	m_CommandListPool = TObjectPool<DX12CommandList>::Create(256);
	m_SemaphorePool = TObjectPool<DX12Semaphore>::Create(256);
	m_ShaderProgramComponentPool = TObjectPool<DX12ShaderProgramComponent>::Create(256);
	m_SamplerComponentPool = TObjectPool<DX12SamplerComponent>::Create(256);
	m_GPUBufferComponentPool = TObjectPool<DX12GPUBufferComponent>::Create(l_renderingCapability.maxBuffers);
	m_OutputMergerTargetPool = TObjectPool<DX12OutputMergerTarget>::Create(128);

	return true;
}

bool DX12RenderingServer::TerminatePool()
{
	delete m_MeshComponentPool;
	delete m_TextureComponentPool;
	delete m_MaterialComponentPool;
	delete m_RenderPassComponentPool;
	delete m_ShaderProgramComponentPool;
	delete m_SamplerComponentPool;
	delete m_GPUBufferComponentPool;
	delete m_PSOPool;
	delete m_CommandListPool;
	delete m_SemaphorePool;

	return true;
}

AddComponent(DX12, Mesh);
AddComponent(DX12, Texture);
AddComponent(DX12, Material);
AddComponent(DX12, RenderPass);
AddComponent(DX12, ShaderProgram);
AddComponent(DX12, Sampler);
AddComponent(DX12, GPUBuffer);

IPipelineStateObject* DX12RenderingServer::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
}

ICommandList* DX12RenderingServer::AddCommandList()
{
	return m_CommandListPool->Spawn();
}

ISemaphore* DX12RenderingServer::AddSemaphore()
{
	return m_SemaphorePool->Spawn();
}

bool DX12RenderingServer::Add(IOutputMergerTarget*& rhs)
{
	rhs = m_OutputMergerTargetPool->Spawn();
	return rhs != nullptr;
}

bool DX12RenderingServer::DeleteMeshComponent(MeshComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshComponent*>(rhs);
	if (l_rhs->m_DefaultHeapBuffer_VB)
		l_rhs->m_DefaultHeapBuffer_VB.ReleaseAndGetAddressOf();

	if (l_rhs->m_DefaultHeapBuffer_IB)
		l_rhs->m_DefaultHeapBuffer_IB.ReleaseAndGetAddressOf();

	if (l_rhs->m_UploadHeapBuffer_VB)
		l_rhs->m_UploadHeapBuffer_VB.ReleaseAndGetAddressOf();

	if (l_rhs->m_UploadHeapBuffer_IB)
		l_rhs->m_UploadHeapBuffer_IB.ReleaseAndGetAddressOf();

	m_MeshComponentPool->Destroy(l_rhs);

	m_initializedMeshes.erase(rhs);

	return true;
}

bool DX12RenderingServer::DeleteTextureComponent(TextureComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);

	if (l_rhs->m_DefaultHeapBuffer)
		l_rhs->m_DefaultHeapBuffer.ReleaseAndGetAddressOf();

	l_rhs->m_UploadHeapBuffers.clear();

	m_TextureComponentPool->Destroy(l_rhs);

	m_initializedTextures.erase(rhs);

	return true;
}

bool DX12RenderingServer::DeleteMaterialComponent(MaterialComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12MaterialComponent*>(rhs);


	m_MaterialComponentPool->Destroy(l_rhs);

	m_initializedMaterials.erase(rhs);

	return true;
}

bool DX12RenderingServer::DeleteRenderPassComponent(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<RenderPassComponent*>(rhs);
	for (size_t i = 0; i < l_rhs->m_CommandLists.size(); i++)
	{
		auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[i]);
		if (!l_commandList)
			continue;

		l_commandList->m_DirectCommandList.ReleaseAndGetAddressOf();
		l_commandList->m_ComputeCommandList.ReleaseAndGetAddressOf();
		l_commandList->m_CopyCommandList.ReleaseAndGetAddressOf();

		m_CommandListPool->Destroy(l_commandList);
	}

	l_rhs->m_CommandLists.clear();

	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_rhs->m_PipelineStateObject);
	if (l_PSO)
	{
		l_PSO->m_PSO.ReleaseAndGetAddressOf();
		m_PSOPool->Destroy(l_PSO);
	}

	DeleteRenderTargets(rhs);

	DeleteShaderProgramComponent(rhs->m_ShaderProgram);

	m_RenderPassComponentPool->Destroy(rhs);

	return true;
}

bool DX12RenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent*>(rhs);
	m_ShaderProgramComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteSamplerComponent(SamplerComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerComponent*>(rhs);
	m_SamplerComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteGPUBufferComponent(GPUBufferComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent*>(rhs);

	for (auto i : l_rhs->m_DeviceMemories)
	{
		if (i->m_DefaultHeapBuffer)
			i->m_DefaultHeapBuffer.ReleaseAndGetAddressOf();
	}

	l_rhs->m_DeviceMemories.clear();

	for (auto i : l_rhs->m_MappedMemories)
	{
		auto l_DX12MappedMemory = reinterpret_cast<DX12MappedMemory*>(i);
		if (l_DX12MappedMemory->m_UploadHeapBuffer)
			l_DX12MappedMemory->m_UploadHeapBuffer.ReleaseAndGetAddressOf();
	}

	m_GPUBufferComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(IPipelineStateObject* rhs)
{
	auto l_rhs = reinterpret_cast<DX12PipelineStateObject*>(rhs);
	l_rhs->m_PSO.ReleaseAndGetAddressOf();
	m_PSOPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(ICommandList* rhs)
{
	auto l_rhs = reinterpret_cast<DX12CommandList*>(rhs);
	l_rhs->m_DirectCommandList.ReleaseAndGetAddressOf();
	l_rhs->m_ComputeCommandList.ReleaseAndGetAddressOf();
	l_rhs->m_CopyCommandList.ReleaseAndGetAddressOf();

	m_CommandListPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(ISemaphore* rhs)
{
	auto l_rhs = reinterpret_cast<DX12Semaphore*>(rhs);

	m_SemaphorePool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(IOutputMergerTarget* rhs)
{
	auto l_rhs = reinterpret_cast<DX12OutputMergerTarget*>(rhs);

	for (auto& j : l_rhs->m_RenderTargets)
	{
		if (j.m_IsOwned)
			DeleteTextureComponent(j.m_Texture);
	}

	l_rhs->m_RenderTargets.clear();

	if (l_rhs->m_DepthStencilRenderTarget.m_IsOwned)
		DeleteTextureComponent(l_rhs->m_DepthStencilRenderTarget.m_Texture);

	m_OutputMergerTargetPool->Destroy(l_rhs);

	return true;
}