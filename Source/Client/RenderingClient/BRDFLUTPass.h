#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace BRDFLUTPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool Terminate();

	RenderPassDataComponent* GetBRDFLUTRPDC();
	RenderPassDataComponent* GetBRDFMSLUTRPDC();

	IResourceBinder* GetBRDFLUT();
	IResourceBinder* GetBRDFMSLUT();
};
