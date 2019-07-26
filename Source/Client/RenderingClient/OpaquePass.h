#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace OpaquePass
{
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* getRPC();
	ShaderProgramComponent* getSPC();
};
