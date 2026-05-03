#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Component/MaterialComponent.h"
#include "../Common/EntityID.h"

namespace Inno
{
	class MaterialResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(MaterialResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		MaterialComponent* Add(const char* name);
		virtual bool Delete(MaterialComponent* ptr);
		MaterialComponent* Find(const char* name);

		void Initialize(MaterialComponent* material, EntityID owner = INVALID_ENTITY);
		bool InitializeComponents();
		bool OnSceneUnloading();

		// Returns the component at the head of the deferred-init queue (the
		// queue is THE source of truth for "pending init work"), or nullptr if
		// the queue is empty. Per no-shadow-state discipline (a86e6e93): the
		// queue's contents ARE the pending-work data, not a shadow of it.
		// Replaces CL 3's pool-iteration shape, which conflated pending-init
		// with scaffolding components whose ObjectStatus is frozen at Created.
		MaterialComponent* GetFirstPendingComponent() const;

		// TASK-213 CL 3.5: deferred-init queue empty signal. Read-only;
		// mirror of GetFirstPendingComponent() == nullptr but cheaper for the
		// CL 4 aggregator's bool-check fast path.
		bool IsDeferredQueueEmpty() const { return m_DeferredQueue.empty(); }

	protected:
		virtual bool InitializeImpl(MaterialComponent* material);

		NamedObjectPool<MaterialComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

	private:
		struct MaterialInitTask
		{
			MaterialInitTask(MaterialComponent* component, EntityID owner = INVALID_ENTITY)
				: m_Component(component), m_Owner(owner) {}

			MaterialComponent* m_Component;
			EntityID m_Owner;
		};

		ThreadSafeQueue<MaterialInitTask> m_DeferredQueue;
	};
}
