#pragma once
#include "../Interface/ISystem.h"

#include "../Common/InnoClassTemplate.h"

namespace Inno
{
	class IRayTracer : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRayTracer);

		virtual bool Execute() = 0;
	};
}