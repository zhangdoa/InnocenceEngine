#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace MotionBlurPass
{
	bool Setup();
	bool Initialize();
	bool Render(GPUResourceComponent* input);
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
	GPUResourceComponent* GetResult();
};
