#pragma once
#include "../Interface/ITestSystem.h"

namespace Inno
{
	class TestSystem : public ITestSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(TestSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool measure(const std::string& functorName, const std::function<void()>& functor) override;
	};
}