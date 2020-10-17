#pragma once
#include "ISystem.h"

namespace Inno
{
	class ITestSystem : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ITestSystem);

		virtual bool measure(const std::string& functorName, const std::function<void()>& functor) = 0;
	};
}