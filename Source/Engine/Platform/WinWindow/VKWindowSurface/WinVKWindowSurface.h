#pragma once
#include "../../../Interface/IWindowSurface.h"

namespace Inno
{
	class WinVKWindowSurface : public IWindowSurface
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(WinVKWindowSurface);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	
	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	};
}