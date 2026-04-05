#include "../GPUBufferResourceService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/FrameManagementService.h"

using namespace Inno;

uint32_t GPUBufferResourceService::GetCurrentFrameIndex()
{
	return g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
}

bool GPUBufferResourceService::Setup(IServiceConfig* systemConfig)
{
	auto l_cap = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();
	m_Pool.Initialize(l_cap.maxBuffers);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "GPUBufferResourceService Setup finished.");
	return true;
}

bool GPUBufferResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "GPUBufferResourceService has been terminated.");
	return true;
}

GPUBufferComponent* GPUBufferResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool GPUBufferResourceService::Delete(GPUBufferComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void GPUBufferResourceService::ForEach(std::function<void(GPUBufferComponent*)> func)
{
	m_Pool.ForEach(func);
}

void GPUBufferResourceService::Initialize(GPUBufferComponent* gpuBuffer)
{
	if (gpuBuffer->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_DeferredQueue.push(gpuBuffer);
	Log(Verbose, "GPUBufferComponent ", gpuBuffer->m_InstanceName, " queued for deferred initialization");
}

void GPUBufferResourceService::Initialize(EntityID entity)
{
	m_DeferredEntityQueue.push(entity);
}

bool GPUBufferResourceService::InitializeComponents()
{
	while (m_DeferredQueue.size() > 0)
	{
		GPUBufferComponent* l_component;
		m_DeferredQueue.tryPop(l_component);

		if (!l_component)
			continue;

		Log(Verbose, "Processing deferred GPU buffer initialization for: ", l_component->m_InstanceName);
		if (InitializeImpl(l_component))
			l_component->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_DeferredQueue.push(std::move(l_component));
	}

	while (m_DeferredEntityQueue.size() > 0)
	{
		EntityID l_entity;
		m_DeferredEntityQueue.tryPop(l_entity);

		if (l_entity == INVALID_ENTITY)
			continue;

		if (m_initializedEntities.count(l_entity) > 0)
			continue;

		if (!InitializeImpl(l_entity))
			m_DeferredEntityQueue.push(l_entity);
	}

	return true;
}

bool GPUBufferResourceService::WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range)
{
	if (gpuBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_size = gpuBuffer->m_TotalSize;
	if (range != SIZE_MAX)
		l_size = range * gpuBuffer->m_ElementSize;

	if (mappedMemory == nullptr)
	{
		Log(Error, "Can't upload data to GPU buffer: ", gpuBuffer->m_InstanceName, " because it's not mapped.");
		return false;
	}

	std::memcpy((char*)mappedMemory->m_Address + startOffset * gpuBuffer->m_ElementSize, sourceMemory, l_size);
	mappedMemory->m_NeedUploadToGPU = true;

	return true;
}

GPUResourceComponent* GPUBufferResourceService::GetTLASBuffer()
{
	return m_TLASBufferComponent;
}
