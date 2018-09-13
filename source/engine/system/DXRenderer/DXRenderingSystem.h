#pragma once

#include "../../component/DXFinalRenderPassSingletonComponent.h"

#include <sstream>

#include "../../component/RenderingSystemSingletonComponent.h"
#include "../../component/WindowSystemSingletonComponent.h"
#include "../../component/AssetSystemSingletonComponent.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#include "DXHeaders.h"

namespace DXRenderingSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	objectStatus getStatus();
};
