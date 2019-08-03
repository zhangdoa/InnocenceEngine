#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace PostTAAPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
