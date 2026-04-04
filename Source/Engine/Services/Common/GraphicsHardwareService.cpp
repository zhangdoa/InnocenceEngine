#include "../GraphicsHardwareService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

using namespace Inno;

bool GraphicsHardwareService::SignalOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType)
{
	if (renderPass == nullptr)
	{
		return SignalOnGPU(static_cast<ISemaphore*>(nullptr), queueType);
	}

	if (renderPass->m_CurrentFrame >= renderPass->m_Semaphores.size())
	{
		Log(Error, "SignalOnGPU: Invalid m_CurrentFrame index %d for RenderPass %s (semaphore count: %d)",
			renderPass->m_CurrentFrame, renderPass->m_InstanceName.c_str(), renderPass->m_Semaphores.size());
		return false;
	}

	auto l_semaphore = renderPass->m_Semaphores[renderPass->m_CurrentFrame];
	return SignalOnGPU(l_semaphore, queueType);
}

bool GraphicsHardwareService::WaitOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	if (renderPass == nullptr)
	{
		return WaitOnGPU(static_cast<ISemaphore*>(nullptr), queueType, semaphoreType);
	}

	if (renderPass->m_CurrentFrame >= renderPass->m_Semaphores.size())
	{
		Log(Error, "WaitOnGPU: Invalid m_CurrentFrame index %d for RenderPass %s (semaphore count: %d)",
			renderPass->m_CurrentFrame, renderPass->m_InstanceName.c_str(), renderPass->m_Semaphores.size());
		return false;
	}

	auto l_semaphore = renderPass->m_Semaphores[renderPass->m_CurrentFrame];
	return WaitOnGPU(l_semaphore, queueType, semaphoreType);
}
