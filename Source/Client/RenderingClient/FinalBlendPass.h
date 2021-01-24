#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace FinalBlendPass
{
	bool Setup();
	bool Initialize();
	bool Render(GPUResourceComponent* input);
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* getSPC();
	GPUResourceComponent* GetResult();
};
