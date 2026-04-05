#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Component/SamplerComponent.h"

namespace Inno
{
	class SamplerResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(SamplerResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		SamplerComponent* Add(const char* name);
		virtual bool Delete(SamplerComponent* ptr);

		void Initialize(SamplerComponent* sampler);

	protected:
		virtual bool InitializeImpl(SamplerComponent* sampler) { return false; }

		NamedObjectPool<SamplerComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
