#pragma once
#include "ISystem.h"

namespace Inno
{
	class IWindowSurfaceConfig : public ISystemConfig
	{
	public:
		void* hInstance;
		void* hwnd;
		void* WindowProc;
	};

	class IWindowSurface : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IWindowSurface);

		virtual bool swapBuffer() = 0;
	};
}