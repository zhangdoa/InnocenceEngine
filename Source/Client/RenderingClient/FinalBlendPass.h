#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace FinalBlendPass
{
	bool Initialize();
	bool PrepareCommandList();

	ShaderProgramComponent* getSPC();
};
