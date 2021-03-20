#pragma once
#include "ISystem.h"

namespace Inno
{
	class IRenderingConfig {};
	class IRenderingClient : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingClient);

		virtual bool Render(IRenderingConfig* renderingConfig = nullptr) = 0;
	};
}