#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace FinalBlendPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList(IResourceBinder* input);
	bool ExecuteCommandList();
	bool Terminate();

	ShaderProgramComponent* getSPC();
};
