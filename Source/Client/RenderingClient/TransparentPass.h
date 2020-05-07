#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace TransparentPass
{
	bool Setup();
	bool Initialize();
	bool Render();
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();

	IResourceBinder* GetResult();
};
