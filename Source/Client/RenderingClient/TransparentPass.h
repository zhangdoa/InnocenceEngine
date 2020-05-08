#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace TransparentPass
{
	bool Setup();
	bool Initialize();
	bool Render(IResourceBinder* canvas);
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();

	IResourceBinder* GetResult();
};
