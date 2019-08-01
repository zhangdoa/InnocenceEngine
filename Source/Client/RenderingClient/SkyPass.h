#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace SkyPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
