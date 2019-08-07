#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace BillboardPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool ExecuteCommandList();
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
