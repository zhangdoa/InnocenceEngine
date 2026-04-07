#pragma once
#include "../../Engine/Interface/ILogicClient.h"

namespace Inno
{
	class ExampleLogicClientImpl;
	class ExampleLogicClient : public ILogicClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(ExampleLogicClient);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const char* GetApplicationName() override;

	private:
		ExampleLogicClientImpl* GetImpl();
		ExampleLogicClientImpl* m_Impl = nullptr;
	};
}
