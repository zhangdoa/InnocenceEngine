#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace BRDFLUTPass
{
	bool Setup();
	bool Initialize();
	bool Render();
	bool Terminate();

	IResourceBinder* GetBRDFLUT();
	IResourceBinder* GetBRDFMSLUT();
};
