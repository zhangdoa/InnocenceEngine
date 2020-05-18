#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace LightPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
	IResourceBinder* GetResult(uint32_t index);
};
