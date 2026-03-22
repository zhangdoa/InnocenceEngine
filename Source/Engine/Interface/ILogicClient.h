#pragma once
#include "IService.h"
#include "../Common/ComponentHeaders.h"

namespace Inno
{
	class ILogicClient : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ILogicClient);

		virtual const char* GetApplicationName() = 0;
	};
}
