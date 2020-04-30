#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace SunShadowPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool Terminate();

	RenderPassDataComponent* GetSunShadowRPDC();
	RenderPassDataComponent* GetBlurRPDCOdd();
	RenderPassDataComponent* GetBlurRPDCEven();
	ShaderProgramComponent* GetSPC();
	IResourceBinder* GetShadowMap();
};
