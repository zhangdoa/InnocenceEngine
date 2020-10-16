#pragma once
#include "../Interface/ITestSystem.h"

class InnoTestSystem : public ITestSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTestSystem);

	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool Update() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	bool measure(const std::string& functorName, const std::function<void()>& functor) override;
};
