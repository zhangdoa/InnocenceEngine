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

		// Residency predicate (no-shadow-state discipline): returns the first
		// component in m_Pool whose status is still ObjectStatus::Created
		// (pre-activation), or nullptr if every live component is Activated.
		// Source of truth is the per-component m_ObjectStatus stamped by
		// InitializeComponents(); no shadow counters.
		MaterialComponent* GetFirstPendingComponent() const;

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
