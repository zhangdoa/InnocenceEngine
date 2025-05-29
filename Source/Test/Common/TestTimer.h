#pragma once
#include "../../Engine/Common/STL14.h"
#include "../../Engine/Common/Timer.h"

namespace Inno
{
	class TestTimer
	{
	public:
		TestTimer() = default;
		~TestTimer() = default;

		void Start();
		void Stop();
		
		double GetDurationMs() const;
		uint64_t GetDurationMicroseconds() const;

		// Static utility functions
		static double MeasureFunction(const std::function<void()>& func);
		
		template<typename Func>
		static double MeasurePerformance(size_t iterations, Func&& func);

	private:
		uint64_t m_StartTime = 0;
		uint64_t m_EndTime = 0;
	};

	// Template implementation (no engine APIs used)
	template<typename Func>
	double TestTimer::MeasurePerformance(size_t iterations, Func&& func)
	{
		TestTimer l_Timer;
		l_Timer.Start();
		for (size_t i = 0; i < iterations; i++)
		{
			func();
		}
		l_Timer.Stop();
		return l_Timer.GetDurationMs();
	}
}
