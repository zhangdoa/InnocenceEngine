#include "../GPUBufferResourceService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/FrameManagementService.h"

using namespace Inno;

uint32_t GPUBufferResourceService::GetCurrentFrameIndex()
{
	return g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
}

// Dedup: log each (buffer, byte_offset) once. Function-local static avoids
// global-init-order risk. Key = (buffer_ptr truncated to 32b) | offset.
std::unordered_set<uint64_t>& GPUBufferResourceService::LoggedUnwrittenKeys()
{
	static std::unordered_set<uint64_t> s_Keys;
	return s_Keys;
}

bool GPUBufferResourceService::LogFirstUnwrittenOnce(GPUBufferComponent* gpuBuffer, size_t byteOffset)
{
	const uint64_t key = (static_cast<uint64_t>(reinterpret_cast<uintptr_t>(gpuBuffer) & 0xFFFFFFFFu) << 32)
		| static_cast<uint64_t>(byteOffset & 0xFFFFFFFFu);
	return LoggedUnwrittenKeys().insert(key).second;
}
bool GPUBufferResourceService::Setup(IServiceConfig* systemConfig)
{
	auto l_cap = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();
	m_Pool.Initialize(l_cap.maxBuffers);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, " Setup finished.");
	return true;
}

bool GPUBufferResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, " terminated.");
	return true;
}

GPUBufferComponent* GPUBufferResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

GPUBufferComponent* GPUBufferResourceService::Find(const char* name)
{
	return m_Pool.Find(name);
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
	Log(Verbose, " ", gpuBuffer->m_InstanceName, " queued for deferred initialization");
}

bool GPUBufferResourceService::InitializeComponents()
{
	while (m_DeferredQueue.size() > 0)
	{
		GPUBufferComponent* l_component;
		m_DeferredQueue.tryPop(l_component);

		if (!l_component)
			continue;

		Log(Verbose, " processing deferred init for: ", l_component->m_InstanceName);
		if (InitializeImpl(l_component))
			l_component->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_DeferredQueue.push(std::move(l_component));
	}

	return true;
}

bool GPUBufferResourceService::WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range)
{
	if (gpuBuffer->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, " WriteMappedMemory rejected for [", gpuBuffer->m_InstanceName, "]: ObjectStatus is ", gpuBuffer->m_ObjectStatus, ", expected Activated.");
		return false;
	}

	auto l_size = gpuBuffer->m_TotalSize;
	if (range != SIZE_MAX)
		l_size = range * gpuBuffer->m_ElementSize;

	if (mappedMemory == nullptr)
	{
		Log(Error, " Can't upload data to GPU buffer: ", gpuBuffer->m_InstanceName, " because it's not mapped.");
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
