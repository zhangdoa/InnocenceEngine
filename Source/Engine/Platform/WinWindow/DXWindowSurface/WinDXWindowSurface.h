#pragma once
#include "../../../Interface/IWindowSurface.h"

namespace Inno
{
	class WinDXWindowSurface : public IWindowSurface
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(WinDXWindowSurface);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	};
}