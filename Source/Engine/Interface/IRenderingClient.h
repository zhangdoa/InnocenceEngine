#pragma once
#include "ISystem.h"

namespace Inno
{
	class IRenderingConfig {};
	class IRenderingClient : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingClient);

		virtual bool PrepareCommands() { return true; }
		virtual bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) = 0;
	};
}