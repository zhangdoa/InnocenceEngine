#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace MotionBlurPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList(RenderPassDataComponent* inputRPDC);
	bool ExecuteCommandList();
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
