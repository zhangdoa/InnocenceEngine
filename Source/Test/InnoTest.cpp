#include "../Engine/Common/InnoContainer.h"
#include "../Engine/Common/InnoMath.h"
#include "../Engine/Core/InnoTimer.h"
#include "../Engine/Core/InnoLogger.h"
#include "../Engine/Core/InnoMemory.h"
#include "../Engine/Core/InnoTaskScheduler.h"

void TestIToA(size_t testCaseCount)
{
	int64_t int64 = std::numeric_limits<int64_t>::max();
	int32_t int32 = std::numeric_limits<int32_t>::max();

	auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_StringFromInt32 = ToString(int32);
		auto l_StringFromInt64 = ToString(int64);
	}

	auto l_Timestamp1 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_StringFromInt32 = std::to_string(int32);
		auto l_StringFromInt64 = std::to_string(int64);
	}

	auto l_Timestamp2 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio = double(l_Timestamp1 - l_StartTime) / double(l_Timestamp2 - l_Timestamp1);

	InnoLogger::Log(LogLevel::Success, "Custom VS STL IToA speed ratio is ", l_SpeedRatio);
}

void TestInnoArray(size_t testCaseCount)
{
	auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	InnoArray<float> l_Array;
	l_Array.reserve(testCaseCount);
	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_Array[i] = (float)i;
	}
	auto l_ArrayCopy = l_Array;

	InnoArray<float> l_Array2;
	l_Array2.reserve(testCaseCount);
	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_Array2.emplace_back((float)i);
	}

	auto l_Timestamp1 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	std::vector<float> l_STLArray;
	l_STLArray.resize(testCaseCount);
	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_STLArray[i] = (float)i;
	}
	auto l_STLArrayCopy = l_STLArray;
	std::vector<float> l_STLArray2;
	l_STLArray2.resize(testCaseCount);
	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_STLArray2.emplace_back((float)i);
	}

	auto l_Timestamp2 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio = double(l_Timestamp1 - l_StartTime) / double(l_Timestamp2 - l_Timestamp1);

	InnoLogger::Log(LogLevel::Success, "Custom VS STL array container speed ratio is ", l_SpeedRatio);
}

void TestInnoMemory(size_t testCaseCount)
{
	std::default_random_engine l_generator;
	std::uniform_int_distribution<unsigned int> l_randomDelta(1, 16384);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_allocateSize = l_randomDelta(l_generator) * l_randomDelta(l_generator);
		auto l_ptr = InnoMemory::Allocate(l_allocateSize);
		InnoLogger::Log(LogLevel::Success, "Memory allocated at ", l_ptr, " for ", l_allocateSize, "Byte(s)");
		InnoMemory::Deallocate(l_ptr);
		InnoLogger::Log(LogLevel::Success, "Memory deallocated at ", l_ptr);
	}
}

class IJob
{
public:
	IJob() = default;
	virtual ~IJob(void) = default;
	IJob(const IJob& rhs) = delete;
	IJob& operator=(const IJob& rhs) = delete;
	IJob(IJob&& other) = default;
	IJob& operator=(IJob&& other) = default;

	virtual void Execute() = 0;
	virtual const char* GetName() = 0;
	virtual void SetUpstreamJob(const IJob* job) = 0;
	virtual bool IsFinished() = 0;
};

template <typename Functor>
class InnoJob : public IJob
{
public:
	InnoJob(Functor&& functor, const char* name)
		:m_Functor{ std::move(functor) }, m_Name{ name }
	{
	}

	~InnoJob() override
	{
		InnoLogger::Log(LogLevel::Success, "Job: ", m_Name, " moved.");
	};

	InnoJob(const InnoJob& rhs) = delete;
	InnoJob& operator=(const InnoJob& rhs) = delete;
	InnoJob(InnoJob&& other)
	{
		InnoLogger::Log(LogLevel::Success, "Job: ", m_Name, " moved.");
	};
	InnoJob& operator=(InnoJob&& other)
	{
		InnoLogger::Log(LogLevel::Success, "Job: ", m_Name, " moved.");
	};
	void Execute() override
	{
		InnoLogger::Log(LogLevel::Verbose, "Job: ", m_Name, " starts executing.");
		if (m_UpstreamJob)
		{
			InnoLogger::Log(LogLevel::Warning, "Job: ", m_Name, " is waiting upstream job: ", m_UpstreamJob->GetName());
			while (!m_UpstreamJob->IsFinished());
			{
			}
		}
		m_IsFinished = false;
		m_Functor();
		m_IsFinished = true;
		InnoLogger::Log(LogLevel::Success, "Job: ", m_Name, " executed.");
	}

	const char* GetName() override
	{
		return m_Name;
	}
	void SetUpstreamJob(const IJob* job) override
	{
		m_UpstreamJob = const_cast<IJob*>(job);
	}
	bool IsFinished() override
	{
		return m_IsFinished;
	}

private:
	Functor m_Functor;
	const char* m_Name;
	IJob* m_UpstreamJob = 0;
	std::atomic_bool m_IsFinished = false;
};

#include<unordered_set>
std::unordered_set<std::unique_ptr<IJob>> m_JobPool;

template <typename Func, typename... Args>
IJob* GenerateJob(const char* name, Func&& func, Args&&... args)
{
	auto BoundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

	using ResultType = std::invoke_result_t<decltype(BoundTask)>;
	using PackagedTask = std::packaged_task<ResultType()>;
	using InnoJobType = InnoJob<PackagedTask>;

	PackagedTask Task{ std::move(BoundTask) };
	auto l_Job = std::make_unique<InnoJobType>(std::move(Task), name);
	auto l_result = l_Job.get();
	m_JobPool.emplace(std::move(l_Job));
	return l_result;
}

void Wait(unsigned int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void ExampleJob()
{
	std::default_random_engine l_generator;
	std::uniform_int_distribution<unsigned int> l_randomDelta(1000, 3000);
	Wait(l_randomDelta(l_generator));
}

void TestJob(size_t testCaseCount)
{
	InnoLogger::Log(LogLevel::Verbose, "Generate test jobs...");
	std::vector<IJob*> l_Jobs;
	std::vector<std::string> l_JobNames;
	l_Jobs.reserve(testCaseCount);
	l_JobNames.reserve(testCaseCount);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_JobName = std::string("TestJob_" + std::to_string(i));
		l_JobNames.emplace_back(l_JobName);
	}

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_Job = GenerateJob(l_JobNames[i].c_str(), &ExampleJob);
		l_Jobs.emplace_back(l_Job);
	}

	// Build random dependencies around all jobs except the first one, we need an almost DAG structure
	InnoLogger::Log(LogLevel::Verbose, "Build random dependencies...");
	for (size_t i = 1; i < testCaseCount; i++)
	{
		std::default_random_engine l_generator;
		std::uniform_int_distribution<unsigned int> l_randomDelta(0, (unsigned int)testCaseCount - 1);
		size_t l_UpstreamJobIndex = l_randomDelta(l_generator);

		// Eliminate self-dependent
		while (l_UpstreamJobIndex == i)
		{
			l_UpstreamJobIndex = l_randomDelta(l_generator);
		}

		l_Jobs[i]->SetUpstreamJob(l_Jobs[l_UpstreamJobIndex]);
	}

	InnoLogger::Log(LogLevel::Verbose, "Dispatch all jobs...");
	std::vector<std::thread> l_threads;
	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_thread = std::thread(&IJob::Execute, l_Jobs[i]);
		l_threads.emplace_back(std::move(l_thread));
	}

	for (size_t i = 0; i < testCaseCount; i++)
	{
		if (l_threads[i].joinable())
		{
			l_threads[i].join();
		}
	}

	InnoLogger::Log(LogLevel::Success, "All Jobs executed.");
}

int main(int argc, char *argv[])
{
	TestIToA(1024);
	TestInnoArray(8192);
	TestInnoMemory(128);
	TestJob(7);

	while (1);
	return 0;
}