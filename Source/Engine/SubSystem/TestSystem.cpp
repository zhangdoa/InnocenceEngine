#include "TestSystem.h"
#include "../Core/InnoLogger.h"
#include "../Core/InnoTimer.h"

using namespace Inno;
namespace Inno
{
	namespace InnoTestSystemNS
	{
		bool Setup();
		bool Initialize();
		bool Update();
		bool Terminate();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	}
}

bool InnoTestSystemNS::Setup()
{
	InnoTestSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoTestSystemNS::Initialize()
{
	if (InnoTestSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		InnoTestSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "TestSystem has been initialized.");
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "TestSystem: Object is not created!");
		return false;
	}
}

bool InnoTestSystemNS::Update()
{
	if (InnoTestSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		InnoTestSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoTestSystemNS::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "TestSystem has been terminated.");

	return true;
}

bool InnoTestSystem::Setup(ISystemConfig* systemConfig)
{
	return InnoTestSystemNS::Setup();
}

bool InnoTestSystem::Initialize()
{
	return InnoTestSystemNS::Initialize();
}

bool InnoTestSystem::Update()
{
	return InnoTestSystemNS::Update();
}

bool InnoTestSystem::Terminate()
{
	return InnoTestSystemNS::Terminate();
}

ObjectStatus InnoTestSystem::GetStatus()
{
	return InnoTestSystemNS::m_ObjectStatus;
}

bool InnoTestSystem::measure(const std::string& functorName, const std::function<void()>& functor)
{
	auto l_startTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	(functor)();

	auto l_endTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_duration = (float)(l_endTime - l_startTime);

	l_duration /= 1000000.0f;

	return true;
}