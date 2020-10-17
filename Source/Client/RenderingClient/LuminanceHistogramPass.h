#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace LuminanceHistogramPass
{
	bool Setup();
	bool Initialize();
	bool Render(IResourceBinder* input);
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();

	GPUBufferDataComponent* GetAverageLuminance();
};
