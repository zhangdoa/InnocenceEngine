#pragma once
#include "ISystem.h"
#include "../Common/ComponentHeaders.h"

namespace Inno
{
	class ILogicClient : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ILogicClient);

		virtual const char* GetApplicationName() = 0;
	};
}
