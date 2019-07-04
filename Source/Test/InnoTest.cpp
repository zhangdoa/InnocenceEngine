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
	virtual IJob* GetUpstreamJob() = 0;
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
	};

	InnoJob(const InnoJob& rhs) = delete;
	InnoJob& operator=(const InnoJob& rhs) = delete;
	InnoJob(InnoJob&& other) = default;
	InnoJob& operator=(InnoJob&& other) = default;

	void Execute() override
	{
		m_IsFinished = false;
		m_Functor();
		m_IsFinished = true;
	}

	const char* GetName() override
	{
		return m_Name.c_str();
	}

	IJob* GetUpstreamJob() override
	{
		return m_UpstreamJob;
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
	IJob* m_UpstreamJob = 0;
	std::atomic_bool m_IsFinished = false;
	FixedSizeString<64> m_Name;
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

class InnoJobScheduler
{
public:
	static bool Setup();
	static bool Initialize();
	static bool Update();
	static bool Terminate();

	static void WaitSync();

	static void AddJobToQueue(IJob* job, int threadID = -1);
};

enum class ThreadStatus { Idle, Busy };

class InnoThread
{
public:
	explicit InnoThread(unsigned int ThreadIndex)
	{
		m_ThreadHandle = new std::thread(&InnoThread::Worker, this, ThreadIndex);
	};

	~InnoThread(void)
	{
		m_Done = true;
		if (m_ThreadHandle->joinable())
		{
			m_ThreadHandle->join();
			delete m_ThreadHandle;
		}
	};

	InnoThread(const InnoThread& rhs) = delete;
	InnoThread& operator=(const InnoThread& rhs) = delete;
	InnoThread(InnoThread&& other) = default;
	InnoThread& operator=(InnoThread&& other) = default;

	ThreadStatus GetStatus() const
	{
		return m_ThreadStatus;
	}

	void AddJobToQueue(IJob* job)
	{
		m_JobQueue.push(job);
	}

private:
	std::string GetThreadID()
	{
		std::stringstream ss;
		ss << m_ID.second;
		return ss.str();
	}

	void ExecuteJob(IJob* job)
	{
#if defined _DEBUG
		auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
#endif
		job->Execute();
#if defined _DEBUG
		auto l_FinishTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
		auto l_Duration = l_FinishTime - l_StartTime;
#endif
	}

	void Worker(unsigned int ThreadIndex)
	{
		auto l_ID = std::this_thread::get_id();
		m_ID = std::make_pair(ThreadIndex, l_ID);
		m_ThreadStatus = ThreadStatus::Idle;
		InnoLogger::Log(LogLevel::Success, "InnoJobScheduler: Thread ", GetThreadID().c_str(), " has been occupied.");

		while (!m_Done)
		{
			m_ThreadStatus = ThreadStatus::Busy;
			IJob* l_job;
			if (m_JobQueue.waitPop(l_job))
			{
				auto l_UpStreamJob = l_job->GetUpstreamJob();
				if (l_UpStreamJob)
				{
					if (!l_UpStreamJob->IsFinished())
					{
						// @TODO: Balance workload around different worker threads
						m_JobQueue.push(l_job);
					}
					else
					{
						ExecuteJob(l_job);
					}
				}
				else
				{
					ExecuteJob(l_job);
				}
			}
			if (m_JobQueue.size() > 0)
			{
			}

			m_ThreadStatus = ThreadStatus::Idle;
		}

		m_ThreadStatus = ThreadStatus::Idle;
		InnoLogger::Log(LogLevel::Success, "InnoJobScheduler: Thread ", GetThreadID().c_str(), " has been released.");
	}

	std::thread* m_ThreadHandle;
	std::pair<unsigned int, std::thread::id> m_ID;
	std::atomic<ThreadStatus> m_ThreadStatus;
	std::atomic_bool m_Done = false;
	ThreadSafeQueue<IJob*> m_JobQueue;
};

namespace InnoJobSchedulerNS
{
	std::atomic_uint m_NumThreads = 0;
	std::vector<std::unique_ptr<InnoThread>> m_Threads;
	std::atomic_bool m_isAllThreadsIdle = true;
}

bool InnoJobScheduler::Setup()
{
	InnoJobSchedulerNS::m_NumThreads = std::max<unsigned int>(std::thread::hardware_concurrency(), 2u);

	InnoJobSchedulerNS::m_Threads.resize(InnoJobSchedulerNS::m_NumThreads);

	try
	{
		for (std::uint32_t i = 0u; i < InnoJobSchedulerNS::m_NumThreads; ++i)
		{
			InnoJobSchedulerNS::m_Threads[i] = std::make_unique<InnoThread>(i);
		}
	}
	catch (...)
	{
		Terminate();
		throw;
	}

	return true;
}

bool InnoJobScheduler::Initialize()
{
	return true;
}

bool InnoJobScheduler::Update()
{
	return true;
}

bool InnoJobScheduler::Terminate()
{
	for (size_t i = 0; i < InnoJobSchedulerNS::m_Threads.size(); i++)
	{
		InnoJobSchedulerNS::m_Threads[i].reset();
	}
	return true;
}

void InnoJobScheduler::WaitSync()
{
	while (!InnoJobSchedulerNS::m_isAllThreadsIdle)
	{
		for (size_t i = 0; i < InnoJobSchedulerNS::m_Threads.size(); i++)
		{
			InnoJobSchedulerNS::m_isAllThreadsIdle = (InnoJobSchedulerNS::m_Threads[i]->GetStatus() == ThreadStatus::Idle);
		}
	}
	InnoLogger::Log(LogLevel::Verbose, "InnoJobScheduler: Reached synchronization point");
}

void InnoJobScheduler::AddJobToQueue(IJob* job, int threadID)
{
	int l_ThreadIndex;
	if (threadID != -1)
	{
		l_ThreadIndex = threadID;
	}
	else
	{
		std::random_device RD;
		std::mt19937 Gen(RD());
		std::uniform_int_distribution<> Dis(0, InnoJobSchedulerNS::m_NumThreads - 1);
		l_ThreadIndex = Dis(Gen);
	}

	InnoJobSchedulerNS::m_Threads[l_ThreadIndex]->AddJobToQueue(job);
}

std::atomic_int FinishedJobCount = 0;
std::atomic_int JobTotalSyncExecutionTime = 0;

void ExampleJob()
{
	std::default_random_engine l_generator;
	std::uniform_int_distribution<unsigned int> l_randomDelta(10, 20);
	auto l_executionTime = l_randomDelta(l_generator);
	JobTotalSyncExecutionTime += l_executionTime;
	std::this_thread::sleep_for(std::chrono::milliseconds(l_executionTime));
	FinishedJobCount++;
}

bool CheckCyclic(std::vector<IJob*> jobs, size_t initialIndex, size_t targetIndex)
{
	auto l_currentJob = jobs[initialIndex];
	auto l_targetJob = jobs[targetIndex];

	bool l_hasCyclic;

	while (l_targetJob)
	{
		l_hasCyclic = (l_currentJob == l_targetJob);

		if (l_hasCyclic)
		{
			break;
		}

		l_targetJob = l_targetJob->GetUpstreamJob();
		if (!l_targetJob)
		{
			l_hasCyclic = false;
			break;
		}
	}

	return l_hasCyclic;
}

void BuildRandomDependencies(const std::vector<IJob*> jobs)
{
	InnoLogger::Log(LogLevel::Verbose, "Build random dependencies...");

	auto l_size = jobs.size();

	std::default_random_engine l_generator;
	std::uniform_int_distribution<unsigned int> l_randomDelta(0, (unsigned int)l_size - 1);

	for (size_t i = 1; i < l_size; i++)
	{
		size_t l_UpstreamJobIndex = l_randomDelta(l_generator);

		// Eliminate cyclic
		while (CheckCyclic(jobs, i, l_UpstreamJobIndex))
		{
			l_UpstreamJobIndex = l_randomDelta(l_generator);
		}

		jobs[i]->SetUpstreamJob(jobs[l_UpstreamJobIndex]);
	}
}

void TestJob(size_t testCaseCount)
{
	InnoJobScheduler::Setup();
	InnoJobScheduler::Initialize();

	InnoLogger::Log(LogLevel::Verbose, "Generate test async jobs...");
	std::vector<IJob*> l_Jobs;
	std::vector<std::string> l_JobNames;
	l_Jobs.reserve(testCaseCount);
	l_JobNames.reserve(testCaseCount);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_JobName = std::string("TestJob_" + std::to_string(i) + "/");
		l_JobNames.emplace_back(l_JobName);
	}

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_Job = GenerateJob(l_JobNames[i].c_str(), &ExampleJob);
		l_Jobs.emplace_back(l_Job);
	}

	// We need a DAG structure
	BuildRandomDependencies(l_Jobs);

	InnoLogger::Log(LogLevel::Verbose, "Dispatch all jobs to async threads...");

	auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		InnoJobScheduler::AddJobToQueue(l_Jobs[i]);
	}

	while (FinishedJobCount != testCaseCount)
	{
	}

	auto l_Timestamp1 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

	auto l_JobTotalAsyncExecutionTime = l_Timestamp1 - l_StartTime;

	auto l_SpeedRatio = double(l_JobTotalAsyncExecutionTime) / double(JobTotalSyncExecutionTime);

	InnoLogger::Log(LogLevel::Success, "Async VS sync job execution speed ratio is ", l_SpeedRatio);
}

int main(int argc, char *argv[])
{
	TestIToA(1024);
	TestInnoArray(8192);
	TestInnoMemory(128);
	TestJob(512);

	while (1);
	return 0;
}