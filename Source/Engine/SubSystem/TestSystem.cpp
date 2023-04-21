#include "TestSystem.h"
#include "../Core/Logger.h"
#include "../Core/Timer.h"

using namespace Inno;
namespace Inno
{
	namespace TestSystemNS
	{
		bool Setup();
		bool Initialize();
		bool Update();
		bool Terminate();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	}
}

bool TestSystemNS::Setup()
{
	TestSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool TestSystemNS::Initialize()
{
	if (TestSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		TestSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		Logger::Log(LogLevel::Success, "TestSystem has been initialized.");
		return true;
	}
	else
	{
		Logger::Log(LogLevel::Error, "TestSystem: Object is not created!");
		return false;
	}
}

bool TestSystemNS::Update()
{
	if (TestSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		TestSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool TestSystemNS::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Logger::Log(LogLevel::Success, "TestSystem has been terminated.");

	return true;
}

bool TestSystem::Setup(ISystemConfig* systemConfig)
{
	return TestSystemNS::Setup();
}

bool TestSystem::Initialize()
{
	return TestSystemNS::Initialize();
}

bool TestSystem::Update()
{
	return TestSystemNS::Update();
}

bool TestSystem::Terminate()
{
	return TestSystemNS::Terminate();
}

ObjectStatus TestSystem::GetStatus()
{
	return TestSystemNS::m_ObjectStatus;
}

bool TestSystem::measure(const std::string& functorName, const std::function<void()>& functor)
{
	auto l_startTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	(functor)();

	auto l_endTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_duration = (float)(l_endTime - l_startTime);

	l_duration /= 1000000.0f;

	return true;
}