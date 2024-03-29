#pragma once
#include "../../Engine/Interface/ILogicClient.h"

namespace Inno
{
	class DefaultLogicClientImpl;
	class DefaultLogicClient : public ILogicClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DefaultLogicClient);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const char* GetApplicationName() override;

	private:
		DefaultLogicClientImpl* GetImpl();
		DefaultLogicClientImpl* m_Impl = nullptr;
	};
}
