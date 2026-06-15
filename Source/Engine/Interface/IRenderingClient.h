#pragma once
#include "IService.h"

namespace Inno
{
	class IRenderingConfig {};
	class IRenderingClient : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingClient);

		virtual bool GetValidationPassed() const { return true; }
	};
}