#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace BRDFLUTPass
{
	bool Setup();
	bool Initialize();
	bool Render();
	bool Terminate();

	IResourceBinder* GetBRDFLUT();
	IResourceBinder* GetBRDFMSLUT();
};
