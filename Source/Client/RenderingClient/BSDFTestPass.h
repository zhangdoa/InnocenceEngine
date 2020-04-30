#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace BSDFTestPass
{
	bool Setup();
	bool Initialize();
	bool Render();
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
