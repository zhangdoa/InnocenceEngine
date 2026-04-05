#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Component/CommandListComponent.h"

namespace Inno
{
	class CommandListResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(CommandListResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		CommandListComponent* Add(const char* name);
		virtual bool Delete(CommandListComponent* ptr);

		void ForEach(std::function<void(CommandListComponent*)> func);

		void Initialize(CommandListComponent* commandList);

	protected:
		virtual bool InitializeImpl(CommandListComponent* commandList) { return false; }

		NamedObjectPool<CommandListComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
