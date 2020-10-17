#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace FinalBlendPass
{
	bool Setup();
	bool Initialize();
	bool Render(IResourceBinder* input);
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* getSPC();
	IResourceBinder* GetResult();
};
