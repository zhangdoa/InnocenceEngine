#pragma once
#include "../Interface/IService.h"

#include "../Common/ClassTemplate.h"

namespace Inno
{
	class IRayTracer : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRayTracer);

		virtual bool Execute() = 0;
	};
}