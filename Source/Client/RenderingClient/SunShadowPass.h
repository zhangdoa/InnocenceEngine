#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace SunShadowPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
