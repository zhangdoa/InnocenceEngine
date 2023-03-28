#pragma once
#include "../../Interface/ISystem.h"

namespace Inno
{
	class IImGuiWindow : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IImGuiWindow);

		virtual bool NewFrame() = 0;
	};
}
