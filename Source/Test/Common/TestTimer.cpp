#include "TestTimer.h"

using namespace Inno;

void TestTimer::Start()
{
	m_StartTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
}

void TestTimer::Stop()
{
	m_EndTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
}

double TestTimer::GetDurationMs() const
{
	return (double)(m_EndTime - m_StartTime) / 1000.0;
}

uint64_t TestTimer::GetDurationMicroseconds() const
{
	return m_EndTime - m_StartTime;
}

double TestTimer::MeasureFunction(const std::function<void()>& func)
{
	TestTimer l_Timer;
	l_Timer.Start();
	func();
	l_Timer.Stop();
	return l_Timer.GetDurationMs();
}
