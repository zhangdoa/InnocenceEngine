#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace OpaquePass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
