#pragma once
#include "ITestSystem.h"

INNO_CONCRETE InnoTestSystem : INNO_IMPLEMENT ITestSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTestSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT bool measure(const std::string& functorName, const std::function<void()>& functor) override;
};
