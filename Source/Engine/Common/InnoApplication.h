#pragma once
#include "InnoType.h"

namespace InnoApplication
{
	bool Setup(
		// Windows: For hInstance
		// macOS: For window bridge
		void* appHook,
		// Windows: For hwnd
		// macOS: For Metal rendering backend bridge
		void* extraHook,
		char* pScmdline
	);
	bool Initialize();
	bool Run();
	bool Terminate();
};
