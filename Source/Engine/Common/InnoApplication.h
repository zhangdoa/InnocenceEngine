#pragma once
#include "InnoType.h"

namespace InnoApplication
{
	bool setup(
		// Windows: For hInstance
		// macOS: For window bridge
		void* appHook,
		// Windows: For hwnd
		// macOS: For Metal rendering backend bridge
		void* extraHook,
		char* pScmdline
	);
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus getStatus();
};
