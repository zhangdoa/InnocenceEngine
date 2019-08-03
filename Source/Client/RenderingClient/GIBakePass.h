#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace GIBakePass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
