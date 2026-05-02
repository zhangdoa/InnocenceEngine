#pragma once
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

		void Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices, EntityID owner = INVALID_ENTITY);
		bool InitializeComponents();
		bool OnSceneUnloading();

		// TASK-213 CL A: deferred-activation drain status. Read-only signal
		// consumed by FrameManagementService::IsSteadyState() to gate auto-
		// test capture frame-counting on a fully-loaded scene. Activation-only
		// tasks re-queue inside InitializeComponents() and drain over N
		// subsequent frames; "queue empty" means no pending mesh activations.
		bool IsDeferredQueueEmpty() const { return m_DeferredQueue.empty(); }

		// Residency predicate (no-shadow-state discipline): returns the first
		// component in m_Pool whose status is still ObjectStatus::Created
		// (pre-activation), or nullptr if every live component is Activated.
		// Source of truth is the per-component m_ObjectStatus stamped by
		// InitializeComponents(); no shadow counters or "is-ready" flags.
		// Caller can log the returned component's m_InstanceName to surface
		// which mesh is still pending, not just how many.
		MeshComponent* GetFirstPendingComponent() const;

		GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle);
		const GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle) const;
		GPUMeshResourceHandle FindMeshResourceByName(const char* name);

	protected:
		virtual bool InitializeImpl(MeshAssetHandle handle, std::vector<Vertex>& vertices, std::vector<Index>& indices) { return false; }
		virtual void ReleaseMeshGPUResourceImpl(MeshAssetHandle handle) = 0;

		NamedObjectPool<MeshComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

		std::vector<GPUMeshResource> m_MeshResources;
		std::vector<uint32_t> m_FreeMeshResourceSlots;
		ThreadSafeUnorderedMap<std::string, GPUMeshResourceHandle> m_MeshResourceLUT;

	private:
		GPUMeshResourceHandle AllocateMeshResource(const char* name, ObjectLifespan lifespan);
		void ReleaseMeshResource(GPUMeshResourceHandle handle);
		void ReleaseAllMeshResources(ObjectLifespan lifespan);

		struct MeshInitTask
		{
			MeshInitTask(MeshComponent* component, std::vector<Vertex>&& vertices, std::vector<Index>&& indices, EntityID owner = INVALID_ENTITY, ObjectLifespan lifespan = ObjectLifespan::Invalid)
				: m_Component(component), m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_Owner(owner), m_Lifespan(lifespan) {}

			MeshComponent* m_Component;
			std::vector<Vertex> m_Vertices;
			std::vector<Index> m_Indices;
			EntityID m_Owner;
			ObjectLifespan m_Lifespan;
		};

		ThreadSafeQueue<MeshInitTask> m_DeferredQueue;
	};
}
