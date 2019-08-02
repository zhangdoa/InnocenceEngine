#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace SSAOPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
