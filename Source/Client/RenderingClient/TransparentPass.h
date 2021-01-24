#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace TransparentPass
{
	bool Setup();
	bool Initialize();
	bool Render(GPUResourceComponent* canvas);
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();

	GPUResourceComponent* GetResult();
};
