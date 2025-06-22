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

	m_PSOPool = TObjectPool<VKPipelineStateObject>::Create(128);
	m_SemaphorePool = TObjectPool<VKSemaphore>::Create(512);

	return true;
}

bool VKRenderingServer::TerminatePool()
{
	delete m_PSOPool;
	delete m_SemaphorePool;

	return true;
}

IPipelineStateObject* VKRenderingServer::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
}

ISemaphore* VKRenderingServer::AddSemaphore()
{
	return m_SemaphorePool->Spawn();
}

bool VKRenderingServer::Add(IOutputMergerTarget*& rhs)
{
	return false;
}

bool VKRenderingServer::Delete(MeshComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Delete(TextureComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Delete(MaterialComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Delete(RenderPassComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Delete(ShaderProgramComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Delete(SamplerComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Delete(GPUBufferComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Delete(IPipelineStateObject *rhs)
{
	auto l_rhs = reinterpret_cast<VKPipelineStateObject*>(rhs);
	
	m_PSOPool->Destroy(l_rhs);

	return true;
}

bool VKRenderingServer::Delete(CommandListComponent *rhs)
{
	if (!rhs || rhs->m_CommandList == 0)
		return true;

	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(rhs->m_CommandList);

	// Free the command buffer back to its pool
	// TODO: We need to store the command pool somewhere to properly free the buffer
	// For now, this is a simplified implementation
	if (l_vkCommandBuffer != VK_NULL_HANDLE)
	{
		// Note: We need the original command pool to properly free the buffer
		// This requires storing the pool reference in the CommandListComponent
		// For now, we'll just set it to null to avoid the leak
		rhs->m_CommandList = 0;
	}

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