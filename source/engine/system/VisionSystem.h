#pragma once
#include "../common/InnoType.h"

#if defined (INNO_RENDERER_OPENGL)
#include "GLRenderer/GLWindowSystem.h"
#include "GLRenderer/GLRenderingSystem.h"
#include "GLRenderer/GLGuiSystem.h"
#elif defined (INNO_RENDERER_DX)
#include "DXRenderer/DXWindowSystem.h"
#include "DXRenderer/DXRenderingSystem.h"
#include "DXRenderer/DXGuiSystem.h"
#endif

#if defined (INNO_RENDERER_OPENGL)
#define WindowSystem GLWindowSystem
#define RenderingSystem GLRenderingSystem
#define GuiSystem GLGuiSystem
#elif defined (INNO_RENDERER_DX)
#define WindowSystem DXWindowSystem
#define RenderingSystem DXRenderingSystem
#define GuiSystem DXGuiSystem
#elif defined (INNO_RENDERER_VULKAN)
#elif defined (INNO_RENDERER_METAL)
#endif

namespace InnoVisionSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	objectStatus getStatus();
};
