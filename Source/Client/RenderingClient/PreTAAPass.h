#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace PreTAAPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
