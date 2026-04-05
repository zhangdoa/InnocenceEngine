#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Component/GPUBufferComponent.h"
#include "../Component/CommandListComponent.h"
#include "../Common/EntityID.h"

namespace Inno
{
	class GPUBufferResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(GPUBufferResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		GPUBufferComponent* Add(const char* name);
		virtual bool Delete(GPUBufferComponent* ptr);

		void ForEach(std::function<void(GPUBufferComponent*)> func);

		void Initialize(GPUBufferComponent* gpuBuffer);
		void Initialize(EntityID entity);
		bool InitializeComponents();

		virtual bool UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) { return false; }
		virtual bool Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) { return false; }

		template<typename T>
		bool Upload(GPUBufferComponent* gpuBuffer, const T* value, size_t startOffset = 0, size_t range = SIZE_MAX);

		template<typename T>
		bool Upload(GPUBufferComponent* gpuBuffer, const std::vector<T>& value, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return Upload(gpuBuffer, &value[0], startOffset, range);
		}

		bool WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range);

		GPUResourceComponent* GetTLASBuffer();
		GPUBufferComponent* GetTLASBufferComponent() { return m_TLASBufferComponent; }
		GPUBufferComponent* GetScratchBufferComponent() { return m_ScratchBufferComponent; }
		GPUBufferComponent* GetRaytracingInstanceBufferComponent() { return m_RaytracingInstanceBufferComponent; }
		std::vector<IRaytracingInstanceDescList*>& GetRaytracingInstanceDescs() { return m_RaytracingInstanceDescs; }
		bool IsTLASReady() const { return m_TLASReady; }
		void SetTLASReady(bool ready) { m_TLASReady = ready; }

	protected:
		virtual bool InitializeImpl(GPUBufferComponent* gpuBuffer) { return false; }
		virtual bool InitializeImpl(EntityID entity) { return false; }
		virtual bool OnSceneLoadingStart() { return false; }

		NamedObjectPool<GPUBufferComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

		GPUBufferComponent* m_TLASBufferComponent = nullptr;
		GPUBufferComponent* m_ScratchBufferComponent = nullptr;
		GPUBufferComponent* m_RaytracingInstanceBufferComponent = nullptr;
		bool m_TLASReady = false;
		std::vector<IRaytracingInstanceDescList*> m_RaytracingInstanceDescs;
		std::unordered_set<EntityID> m_initializedEntities;

	private:
		uint32_t GetCurrentFrameIndex();

		ThreadSafeQueue<GPUBufferComponent*> m_DeferredQueue;
		ThreadSafeQueue<EntityID> m_DeferredEntityQueue;
	};

	template<typename T>
	bool GPUBufferResourceService::Upload(GPUBufferComponent* gpuBuffer, const T* value, size_t startOffset, size_t range)
	{
		auto l_currentFrame = GetCurrentFrameIndex();
		auto l_mappedMemory = gpuBuffer->m_MappedMemories[l_currentFrame];
		return WriteMappedMemory(gpuBuffer, l_mappedMemory, value, startOffset, range);
	}
}
