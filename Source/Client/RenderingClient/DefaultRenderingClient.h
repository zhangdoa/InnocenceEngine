#pragma once
#include "../../Engine/Core/IRenderingClient.h"

class DefaultRenderingClient : public IRenderingClient
{
	// Inherited via IRenderingClient
	virtual bool Setup() override;
	virtual bool Initialize() override;
	virtual bool PrepareCommandList() override;
	virtual bool ExecuteCommandList() override;
	virtual bool Terminate() override;
};