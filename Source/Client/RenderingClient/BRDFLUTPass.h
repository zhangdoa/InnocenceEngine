#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace BRDFLUTPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool ExecuteCommandList();

	IResourceBinder* GetBRDFLUT();
	IResourceBinder* GetBRDFMSLUT();
};
