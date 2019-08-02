#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace BRDFLUTPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	IResourceBinder* GetBRDFLUT();
	IResourceBinder* GetBRDFMSLUT();
};
