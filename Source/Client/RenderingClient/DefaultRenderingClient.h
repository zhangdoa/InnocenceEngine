#pragma once
#include "../../Engine/Interface/IRenderingClient.h"

namespace Inno
{
	class DefaultRenderingClient : public IRenderingClient
	{
		// Inherited via IRenderingClient
		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Render() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}