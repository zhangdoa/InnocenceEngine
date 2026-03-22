#pragma once
#include "../../Interface/IService.h"

namespace Inno
{
	class IImGuiWindow : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IImGuiWindow);

		virtual bool NewFrame() = 0;
	};
}
