#include "../GraphicsHardwareService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

using namespace Inno;

bool GraphicsHardwareService::Setup(IServiceConfig* systemConfig)
{
	if (!CreateHardwareResources())
	{
		Log(Error, "GraphicsHardwareService: CreateHardwareResources() failed.");
		return false;
	}

	return true;
}

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

bool GraphicsHardwareService::BeginGpuPass(CommandListComponent* commandList, const char* name, GPUEngineType queueType, uint32_t color)
{
	// PIX event first, timer second: the timer slot picks the smallest
	// possible interval, while the PIX event nests around any
	// non-instrumented work between the calls.
	bool l_eventOk = BeginGpuEvent(commandList, name, color);
	bool l_timerOk = BeginGpuTimer(commandList, name, queueType);
	return l_eventOk && l_timerOk;
}

bool GraphicsHardwareService::EndGpuPass(CommandListComponent* commandList, const char* name, GPUEngineType queueType)
{
	// End in reverse order of Begin so the PIX event always fully
	// encloses the timer interval.
	bool l_timerOk = EndGpuTimer(commandList, name, queueType);
	bool l_eventOk = EndGpuEvent(commandList);
	return l_eventOk && l_timerOk;
}
