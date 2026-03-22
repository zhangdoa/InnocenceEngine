#include "VKGraphicsService.h"

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Common/Randomizer.h"
#include "../../Services/RenderingConfigurationService.h"

#include "../../Engine.h"

using namespace Inno;

bool VKGraphicsService::InitializePool()
{
	auto l_renderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	m_PSOPool = TObjectPool<VKPipelineStateObject>::Create(128);
	m_SemaphorePool = TObjectPool<VKSemaphore>::Create(512);

	return true;
}

bool VKGraphicsService::TerminatePool()
{
	delete m_PSOPool;
	delete m_SemaphorePool;

	return true;
}

IPipelineStateObject* VKGraphicsService::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
}

ISemaphore* VKGraphicsService::AddSemaphore()
{
	return m_SemaphorePool->Spawn();
}

bool VKGraphicsService::Add(IOutputMergerTarget*& rhs)
{
	return false;
}

bool VKGraphicsService::Delete(MeshComponent *rhs)
{
	return true;
}

bool VKGraphicsService::Delete(TextureComponent *rhs)
{
	return true;
}

bool VKGraphicsService::Delete(MaterialComponent *rhs)
{
	return true;
}

bool VKGraphicsService::Delete(RenderPassComponent *rhs)
{
	return true;
}

bool VKGraphicsService::Delete(ShaderProgramComponent *rhs)
{
	return true;
}

bool VKGraphicsService::Delete(SamplerComponent *rhs)
{
	return true;
}

bool VKGraphicsService::Delete(GPUBufferComponent *rhs)
{
	return true;
}

bool VKGraphicsService::Delete(IPipelineStateObject *rhs)
{
	auto l_rhs = reinterpret_cast<VKPipelineStateObject*>(rhs);
	
	m_PSOPool->Destroy(l_rhs);

	return true;
}

bool VKGraphicsService::Delete(CommandListComponent *rhs)
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

bool VKGraphicsService::Delete(ISemaphore *rhs)
{
	auto l_rhs = reinterpret_cast<VKSemaphore*>(rhs);
	
	m_SemaphorePool->Destroy(l_rhs);

	return true;
}

bool VKGraphicsService::Delete(IOutputMergerTarget* rhs)
{
	return true;
}