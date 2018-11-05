#include "CoreSystem.h"
#include "TimeSystem.h"

INNO_PRIVATE_SCOPE CoreSystemNS
{
	std::unique_ptr<ITimeSystem> m_TimeSystem;
}

INNO_SYSTEM_EXPORT bool CoreSystem::setup()
{
	CoreSystemNS::m_TimeSystem = std::make_unique<InnoTimeSystem>();
	if (CoreSystemNS::m_TimeSystem.get())
	{
		CoreSystemNS::m_TimeSystem.get()->setup();
		return true;
	}
	else
	{
		return false;
	}
}

INNO_SYSTEM_EXPORT bool CoreSystem::initialize()
{
	return INNO_SYSTEM_EXPORT bool();
}

INNO_SYSTEM_EXPORT bool CoreSystem::update()
{
	return INNO_SYSTEM_EXPORT bool();
}

INNO_SYSTEM_EXPORT bool CoreSystem::terminate()
{
	return INNO_SYSTEM_EXPORT bool();
}

INNO_SYSTEM_EXPORT ITimeSystem * CoreSystem::getTimeSystem()
{
	return nullptr;
}

INNO_SYSTEM_EXPORT objectStatus CoreSystem::getStatus()
{
	return INNO_SYSTEM_EXPORT objectStatus();
}
