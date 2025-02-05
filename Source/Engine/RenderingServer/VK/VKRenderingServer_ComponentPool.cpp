#include "VKRenderingServer.h"

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Common/Randomizer.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/EntityManager.h"

#include "../../Engine.h"

using namespace Inno;

bool VKRenderingServer::InitializePool()
{
	auto l_renderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	m_MeshComponentPool = TObjectPool<VKMeshComponent>::Create(l_renderingCapability.maxMeshes);
	m_TextureComponentPool = TObjectPool<VKTextureComponent>::Create(l_renderingCapability.maxTextures);
	m_MaterialComponentPool = TObjectPool<VKMaterialComponent>::Create(l_renderingCapability.maxMaterials);
	m_RenderPassComponentPool = TObjectPool<VKRenderPassComponent>::Create(128);
	m_PSOPool = TObjectPool<VKPipelineStateObject>::Create(128);
	m_CommandListPool = TObjectPool<VKCommandList>::Create(256);
	m_SemaphorePool = TObjectPool<VKSemaphore>::Create(512);
	m_ShaderProgramComponentPool = TObjectPool<VKShaderProgramComponent>::Create(256);
	m_SamplerComponentPool = TObjectPool<VKSamplerComponent>::Create(256);
	m_GPUBufferComponentPool = TObjectPool<VKGPUBufferComponent>::Create(256);

	return true;
}

bool VKRenderingServer::TerminatePool()
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

AddComponent(VK, Mesh);
AddComponent(VK, Texture);
AddComponent(VK, Material);
AddComponent(VK, RenderPass);
AddComponent(VK, ShaderProgram);
AddComponent(VK, Sampler);
AddComponent(VK, GPUBuffer);

IPipelineStateObject* VKRenderingServer::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
}

ICommandList* VKRenderingServer::AddCommandList()
{
	return m_CommandListPool->Spawn();
}

ISemaphore* VKRenderingServer::AddSemaphore()
{
	return m_SemaphorePool->Spawn();
}

bool VKRenderingServer::Add(IOutputMergerTarget*& rhs)
{
	return false;
}

bool VKRenderingServer::DeleteMeshComponent(MeshComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteTextureComponent(TextureComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteMaterialComponent(MaterialComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteRenderPassComponent(RenderPassComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteSamplerComponent(SamplerComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteGPUBufferComponent(GPUBufferComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Delete(IPipelineStateObject *rhs)
{
	auto l_rhs = reinterpret_cast<VKPipelineStateObject*>(rhs);
	
	m_PSOPool->Destroy(l_rhs);

	return true;
}

bool VKRenderingServer::Delete(ICommandList *rhs)
{
	auto l_rhs = reinterpret_cast<VKCommandList*>(rhs);

	m_CommandListPool->Destroy(l_rhs);

	return true;
}

bool VKRenderingServer::Delete(ISemaphore *rhs)
{
	auto l_rhs = reinterpret_cast<VKSemaphore*>(rhs);
	
	m_SemaphorePool->Destroy(l_rhs);

	return true;
}

bool VKRenderingServer::Delete(IOutputMergerTarget* rhs)
{
	return true;
}