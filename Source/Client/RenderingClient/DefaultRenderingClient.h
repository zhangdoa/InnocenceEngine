#pragma once
#include "../../Engine/Interface/IRenderingClient.h"

namespace Inno
{
	class DefaultRenderingClientImpl;
	class DefaultRenderingClient : public IRenderingClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DefaultRenderingClient);

		// Inherited via IRenderingClient
		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Prepare() override;
		bool Render(IRenderingConfig* renderingConfig = nullptr) override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

	private:
		DefaultRenderingClientImpl* m_Impl;
	};
}