#include "DX12RenderingServer.h"

#include "../../Common/Randomizer.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/EntityManager.h"

#include "../../Engine.h"

using namespace Inno;

bool DX12RenderingServer::InitializePool()
{
	IRenderingServer::InitializePool();

	m_PSOPool = TObjectPool<DX12PipelineStateObject>::Create(128);
	m_CommandListPool = TObjectPool<DX12CommandList>::Create(256);
	m_SemaphorePool = TObjectPool<DX12Semaphore>::Create(256);
	m_OutputMergerTargetPool = TObjectPool<DX12OutputMergerTarget>::Create(128);

	return true;
}

bool DX12RenderingServer::TerminatePool()
{
	IRenderingServer::TerminatePool();

	m_MeshVertexBuffers_Upload.clear();
	m_MeshVertexBuffers_Default.clear();
	m_MeshIndexBuffers_Upload.clear();
	m_MeshIndexBuffers_Default.clear();
	m_MeshBLAS.clear();
	m_MeshScratchBuffers.clear();
	m_TextureBuffers_Upload.clear();
	m_TextureBuffers_Default.clear();

	delete m_PSOPool;
	delete m_CommandListPool;
	delete m_SemaphorePool;
	delete m_OutputMergerTargetPool;

	return true;
}

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

bool DX12RenderingServer::Delete(MeshComponent* rhs)
{
	auto componentUUID = rhs->m_UUID;

	auto vertexUploadIt = m_MeshVertexBuffers_Upload.find(componentUUID);
	if (vertexUploadIt != m_MeshVertexBuffers_Upload.end()) {
		if (vertexUploadIt->second) vertexUploadIt->second.Reset();
		m_MeshVertexBuffers_Upload.erase(vertexUploadIt);
	}

	auto vertexDefaultIt = m_MeshVertexBuffers_Default.find(componentUUID);
	if (vertexDefaultIt != m_MeshVertexBuffers_Default.end()) {
		if (vertexDefaultIt->second) vertexDefaultIt->second.Reset();
		m_MeshVertexBuffers_Default.erase(vertexDefaultIt);
	}

	auto indexUploadIt = m_MeshIndexBuffers_Upload.find(componentUUID);
	if (indexUploadIt != m_MeshIndexBuffers_Upload.end()) {
		if (indexUploadIt->second) indexUploadIt->second.Reset();
		m_MeshIndexBuffers_Upload.erase(indexUploadIt);
	}

	auto indexDefaultIt = m_MeshIndexBuffers_Default.find(componentUUID);
	if (indexDefaultIt != m_MeshIndexBuffers_Default.end()) {
		if (indexDefaultIt->second) indexDefaultIt->second.Reset();
		m_MeshIndexBuffers_Default.erase(indexDefaultIt);
	}

	auto blasIt = m_MeshBLAS.find(componentUUID);
	if (blasIt != m_MeshBLAS.end()) {
		if (blasIt->second) blasIt->second.Reset();
		m_MeshBLAS.erase(blasIt);
	}

	auto scratchIt = m_MeshScratchBuffers.find(componentUUID);
	if (scratchIt != m_MeshScratchBuffers.end()) {
		if (scratchIt->second) scratchIt->second.Reset();
		m_MeshScratchBuffers.erase(scratchIt);
	}

	return true;
}

bool DX12RenderingServer::Delete(TextureComponent* rhs)
{
	if (!rhs)
	{
		return false;
	}

	auto componentUUID = rhs->m_UUID;

	// Clean up texture upload/default buffer if it exists
	auto uploadIt = m_TextureBuffers_Upload.find(componentUUID);
	if (uploadIt != m_TextureBuffers_Upload.end()) {
		if (uploadIt->second) uploadIt->second.Reset();
		m_TextureBuffers_Upload.erase(uploadIt);
	}

	auto defaultIt = m_TextureBuffers_Default.find(componentUUID);
	if (defaultIt != m_TextureBuffers_Default.end()) {
		if (defaultIt->second) defaultIt->second.Reset();
		m_TextureBuffers_Default.erase(defaultIt);
	}

	// Clear GPU resources safely
	try {
		rhs->m_GPUResources.clear();
		rhs->m_ReadHandles.clear();
		rhs->m_WriteHandles.clear();
	}
	catch (...) {
		// Handle potential access violations during cleanup
	}

	// Remove from initialized textures with safety check
	auto textureIt = m_initializedTextures.find(rhs);
	if (textureIt != m_initializedTextures.end()) {
		m_initializedTextures.erase(textureIt);
	}

	return true;
}

bool DX12RenderingServer::Delete(MaterialComponent* rhs)
{
	m_initializedMaterials.erase(rhs);

	return true;
}

bool DX12RenderingServer::Delete(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<RenderPassComponent*>(rhs);
	for (size_t i = 0; i < l_rhs->m_CommandLists.size(); i++)
	{
		auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[i]);
		if (!l_commandList)
			continue;

		l_commandList->m_DirectCommandList.Reset();
		l_commandList->m_ComputeCommandList.Reset();
		l_commandList->m_CopyCommandList.Reset();

		m_CommandListPool->Destroy(l_commandList);
	}

	l_rhs->m_CommandLists.clear();

	DeleteRenderTargets(rhs);

	Delete(rhs->m_ShaderProgram);

	//m_RenderPassComponentPool->Destroy(rhs);

	return true;
}

bool DX12RenderingServer::Delete(ShaderProgramComponent* rhs)
{
	//auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent*>(rhs);
	//m_ShaderProgramComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(SamplerComponent* rhs)
{
	//auto l_rhs = reinterpret_cast<DX12SamplerComponent*>(rhs);
	//m_SamplerComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(GPUBufferComponent* rhs)
{
	for (auto i : rhs->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		if (l_DX12DeviceMemory->m_DefaultHeapBuffer)
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Reset();
	}

	rhs->m_DeviceMemories.clear();

	for (auto i : rhs->m_MappedMemories)
	{
		auto l_DX12MappedMemory = reinterpret_cast<DX12MappedMemory*>(i);
		if (l_DX12MappedMemory->m_UploadHeapBuffer)
			l_DX12MappedMemory->m_UploadHeapBuffer.Reset();
	}

	//m_GPUBufferComponentPool->Destroy(rhs);

	return true;
}

bool DX12RenderingServer::Delete(IPipelineStateObject* rhs)
{
	auto l_rhs = reinterpret_cast<DX12PipelineStateObject*>(rhs);
	l_rhs->m_PSO.Reset();
	m_PSOPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(ICommandList* rhs)
{
	auto l_rhs = reinterpret_cast<DX12CommandList*>(rhs);
	l_rhs->m_DirectCommandList.Reset();
	l_rhs->m_ComputeCommandList.Reset();
	l_rhs->m_CopyCommandList.Reset();

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

	for (auto& j : l_rhs->m_ColorOutputs)
	{
		if (j)
			Delete(j);
	}

	l_rhs->m_ColorOutputs.clear();

	if (l_rhs->m_DepthStencilOutput)
		Delete(l_rhs->m_DepthStencilOutput);

	l_rhs->m_DepthStencilOutput = nullptr;

	m_OutputMergerTargetPool->Destroy(l_rhs);

	return true;
}