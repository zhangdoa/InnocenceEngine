#pragma once
#include "../../../Interface/IWindowSurface.h"

namespace Inno
{
	class WinGLWindowSurface : public IWindowSurface
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(WinGLWindowSurface);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool swapBuffer() override;
	};
}