#pragma once
#include "../Common/Array.h"
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/GPUMeshResource.h"
#include "../Common/EntityID.h"
#include "../Component/MeshComponent.h"

namespace Inno
{
	class MeshResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(MeshResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		MeshComponent* Add(const char* name);
		virtual bool Delete(MeshComponent* ptr);

		void Initialize(MeshComponent* mesh, Inno::Array<Vertex>& vertices, Inno::Array<Index>& indices, EntityID owner = INVALID_ENTITY);
		bool InitializeComponents();
		bool OnSceneUnloading();

		// True when no mesh-activation tasks remain in the deferred queue. The drain
		// can span multiple frames because activation-only tasks re-queue inside
		// InitializeComponents() — callers gating on a fully-loaded scene must poll.
		bool IsDeferredQueueEmpty() const { return m_DeferredQueue.empty(); }

		GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle);
		const GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle) const;
		GPUMeshResourceHandle FindMeshResourceByName(const char* name);

	protected:
		virtual bool InitializeImpl(MeshAssetHandle handle, Inno::Array<Vertex>& vertices, Inno::Array<Index>& indices) { return false; }
		virtual void ReleaseMeshGPUResourceImpl(MeshAssetHandle handle) = 0;

		NamedObjectPool<MeshComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

		Inno::Array<GPUMeshResource> m_MeshResources;
		Inno::Array<uint32_t> m_FreeMeshResourceSlots;
		ThreadSafeUnorderedMap<std::string, GPUMeshResourceHandle> m_MeshResourceLUT;

	private:
		GPUMeshResourceHandle AllocateMeshResource(const char* name, ObjectLifespan lifespan);
		void ReleaseMeshResource(GPUMeshResourceHandle handle);
		void ReleaseAllMeshResources(ObjectLifespan lifespan);

		struct MeshInitTask
		{
			MeshInitTask(MeshComponent* component, Inno::Array<Vertex>&& vertices, Inno::Array<Index>&& indices, EntityID owner = INVALID_ENTITY, ObjectLifespan lifespan = ObjectLifespan::Invalid)
				: m_Component(component), m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_Owner(owner), m_Lifespan(lifespan) {}

			MeshComponent* m_Component;
			Inno::Array<Vertex> m_Vertices;
			Inno::Array<Index> m_Indices;
			EntityID m_Owner;
			ObjectLifespan m_Lifespan;
		};

		ThreadSafeQueue<MeshInitTask> m_DeferredQueue;
	};
}
