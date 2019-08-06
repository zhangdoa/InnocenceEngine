#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace BRDFLUTPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool ExecuteCommandList();
	bool Terminate();

	IResourceBinder* GetBRDFLUT();
	IResourceBinder* GetBRDFMSLUT();
};
