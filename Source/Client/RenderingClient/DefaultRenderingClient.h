#pragma once
#include "../../Engine/Interface/IRenderingClient.h"

class DefaultRenderingClient : public IRenderingClient
{
	// Inherited via IRenderingClient
	bool Setup() override;
	bool Initialize() override;
	bool PrepareCommandList() override;
	bool ExecuteCommandList() override;
	bool Terminate() override;
};