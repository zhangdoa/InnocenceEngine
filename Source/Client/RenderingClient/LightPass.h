#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace LightPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
