#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace TAAPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
