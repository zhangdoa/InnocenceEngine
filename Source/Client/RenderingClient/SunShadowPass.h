#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace SunShadowPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool Terminate();

	RenderPassDataComponent* GetGeometryProcessRPDC();
	RenderPassDataComponent* GetBlurRPDCOdd();
	RenderPassDataComponent* GetBlurRPDCEven();
	ShaderProgramComponent* GetSPC();
	IResourceBinder* GetShadowMap();
};
