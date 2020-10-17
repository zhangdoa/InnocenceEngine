#pragma once
#include "IImGuiWindow.h"

namespace Inno
{
	class ImGuiWindowWin : public IImGuiWindow
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWindowWin);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool NewFrame() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;
	};
}