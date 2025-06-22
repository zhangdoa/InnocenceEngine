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
	delete m_SemaphorePool;
	delete m_OutputMergerTargetPool;

	return true;
}

IPipelineStateObject* DX12RenderingServer::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
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

bool DX12RenderingServer::Delete(MeshComponent* mesh)
{
	auto componentUUID = mesh->m_UUID;

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

bool DX12RenderingServer::Delete(TextureComponent* texture)
{
	if (!texture)
	{
		return false;
	}

	auto componentUUID = texture->m_UUID;

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
		texture->m_GPUResources.clear();
		texture->m_ReadHandles.clear();
		texture->m_WriteHandles.clear();
	}
	catch (...) {
		// Handle potential access violations during cleanup
	}

	// Remove from initialized textures with safety check
	auto textureIt = m_initializedTextures.find(texture);
	if (textureIt != m_initializedTextures.end()) {
		m_initializedTextures.erase(textureIt);
	}

	return true;
}

bool DX12RenderingServer::Delete(MaterialComponent* material)
{
	m_initializedMaterials.erase(material);

	return true;
}

bool DX12RenderingServer::Delete(RenderPassComponent* renderPass)
{
	// Command lists are now created dynamically and managed separately - no cleanup needed

	DeleteRenderTargets(renderPass);

	Delete(renderPass->m_ShaderProgram);

	//m_RenderPassComponentPool->Destroy(renderPass);

	return true;
}

bool DX12RenderingServer::Delete(ShaderProgramComponent* shaderProgram)
{
	//auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent*>(shaderProgram);
	//m_ShaderProgramComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(SamplerComponent* sampler)
{
	//auto l_rhs = reinterpret_cast<DX12SamplerComponent*>(sampler);
	//m_SamplerComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::Delete(GPUBufferComponent* gpuBuffer)
{
	for (auto i : gpuBuffer->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		if (l_DX12DeviceMemory->m_DefaultHeapBuffer)
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Reset();
	}

	gpuBuffer->m_DeviceMemories.clear();

	for (auto i : gpuBuffer->m_MappedMemories)
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

bool DX12RenderingServer::Delete(CommandListComponent* commandList)
{
	auto l_dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (l_dx12CommandList)
	{
		l_dx12CommandList->Release();
	}

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