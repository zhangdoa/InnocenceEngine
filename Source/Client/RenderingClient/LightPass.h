#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace LightPass
{
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* getRPC();
	ShaderProgramComponent* getSPC();
};
