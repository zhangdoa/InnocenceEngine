#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace SSAOPass
{
	bool Setup();
	bool Initialize();
	bool Render();
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
