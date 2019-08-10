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

void TestArray(size_t testCaseCount)
{
	auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	Array<float> l_Array;
	l_Array.reserve(testCaseCount);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_Array.emplace_back((float)i);
	}

	auto l_ArrayCopy = l_Array;

	Array<float> l_Array2;
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
	static std::vector<unsigned int> l_objectSizes = { 64, 128, 256, 512, 1024, 2048, 4096, 8192 };
	std::default_random_engine l_generator;
	std::uniform_int_distribution<unsigned int> l_randomDelta(0, 7);
	auto l_objectSize = l_objectSizes[l_randomDelta(l_generator)];

	std::vector<void*> l_objectInCustomPool(testCaseCount);
	std::vector<void*> l_objectRaw(testCaseCount);

	auto l_objectPool = InnoMemory::CreateObjectPool(l_objectSize, (unsigned int)testCaseCount);

	auto l_StartTime1 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_ptr = l_objectPool->Spawn();
		l_objectInCustomPool[i] = l_ptr;
	}

	auto l_Timestamp1 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_ptr = malloc(l_objectSize);
		l_objectRaw[i] = l_ptr;
	}

	auto l_Timestamp2 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio1 = double(l_Timestamp1 - l_StartTime1) / double(l_Timestamp2 - l_Timestamp1);

	InnoLogger::Log(LogLevel::Success, "Custom object pool allocation VS malloc() speed ratio is ", l_SpeedRatio1);

	auto l_StartTime2 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_objectPool->Destroy(l_objectInCustomPool[i]);
	}

	auto l_Timestamp3 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		free(l_objectRaw[i]);
	}

	auto l_Timestamp4 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio2 = double(l_Timestamp3 - l_StartTime2) / double(l_Timestamp4 - l_Timestamp3);

	InnoLogger::Log(LogLevel::Success, "Custom object pool deallocation VS free() speed ratio is ", l_SpeedRatio1);
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

struct JobReport
{
	const char* m_JobName = 0;
	unsigned int m_ThreadID = 0;
	unsigned long long m_ExecutionTime = 0;
};

enum class ThreadState { Idle, Busy };

#include<unordered_set>
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
		m_JobQueue.invalidate();
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

	ThreadState GetStatus() const
	{
		return m_ThreadStatus;
	}

	void AddJobToQueue(IJob* job)
	{
		m_JobQueue.push(job);
	}

private:
	std::string GetThreadID();

	void ExecuteJob(IJob* job);

	void Worker(unsigned int ThreadIndex);

	std::thread* m_ThreadHandle;
	std::pair<unsigned int, std::thread::id> m_ID;
	std::atomic<ThreadState> m_ThreadStatus;
	std::atomic_bool m_Done = false;
	ThreadSafeQueue<IJob*> m_JobQueue;
};

namespace InnoJobSchedulerNS
{
	std::atomic_uint m_NumThreads = 0;
	std::vector<std::unique_ptr<InnoThread>> m_Threads;
	std::atomic_bool m_isAllThreadsIdle = true;
	std::unordered_set<std::unique_ptr<IJob>> m_JobPool;
	RingBuffer<JobReport, true>* m_JobReports;
	std::shared_mutex m_Mutex;
}

using namespace InnoJobSchedulerNS;

inline std::string InnoThread::GetThreadID()
{
	std::stringstream ss;
	ss << m_ID.second;
	return ss.str();
}

std::atomic_int t = 0;

inline void InnoThread::ExecuteJob(IJob * job)
{
#if defined _DEBUG
	auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
#endif
	job->Execute();
#if defined _DEBUG
	auto l_FinishTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
	auto l_Duration = l_FinishTime - l_StartTime;

	auto l_report = JobReport{ job->GetName(), m_ID.first, l_Duration };
	std::unique_lock<std::shared_mutex> lock{ m_Mutex };
	m_JobReports->emplace_back(l_report);
#endif
}

inline void InnoThread::Worker(unsigned int ThreadIndex)
{
	auto l_ID = std::this_thread::get_id();
	m_ID = std::make_pair(ThreadIndex, l_ID);
	m_ThreadStatus = ThreadState::Idle;
	InnoLogger::Log(LogLevel::Success, "InnoJobScheduler: Thread ", GetThreadID().c_str(), " has been occupied.");

	while (!m_Done)
	{
		m_ThreadStatus = ThreadState::Busy;
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
		m_ThreadStatus = ThreadState::Idle;
	}

	m_ThreadStatus = ThreadState::Idle;
	InnoLogger::Log(LogLevel::Success, "InnoJobScheduler: Thread ", GetThreadID().c_str(), " has been released.");
}

class InnoJobScheduler
{
public:
	static bool Setup();
	static bool Initialize();
	static bool Update();
	static bool Terminate();

	static void WaitSync();

	static void AddJob(std::unique_ptr<IJob>&& job);
	static void ExecuteJob(IJob* job, int threadID = -1);
};

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
	InnoJobScheduler::AddJob(std::move(l_Job));
	return l_result;
}

bool InnoJobScheduler::Setup()
{
	m_NumThreads = std::max<unsigned int>(std::thread::hardware_concurrency(), 2u);

	m_Threads.resize(m_NumThreads);

	try
	{
		for (std::uint32_t i = 0u; i < m_NumThreads; ++i)
		{
			m_Threads[i] = std::make_unique<InnoThread>(i);
		}
	}
	catch (...)
	{
		Terminate();
		throw;
	}
	m_JobReports = new RingBuffer<JobReport, true>();
	m_JobReports->reserve(256);

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
	for (size_t i = 0; i < m_NumThreads; i++)
	{
		m_Threads[i].reset();
	}

	for (size_t i = 0; i < m_JobReports->size(); i++)
	{
		InnoLogger::Log(LogLevel::Verbose, "InnoJobScheduler: JobReport: ", m_JobReports->operator[](i).m_JobName, " on thread ", m_JobReports->operator[](i).m_ThreadID, " executed in ", (float)m_JobReports->operator[](i).m_ExecutionTime / 1000.0f, " ms.");
	};

	delete m_JobReports;

	return true;
}

void InnoJobScheduler::WaitSync()
{
	while (!m_isAllThreadsIdle)
	{
		for (size_t i = 0; i < m_Threads.size(); i++)
		{
			m_isAllThreadsIdle = (m_Threads[i]->GetStatus() == ThreadState::Idle);
		}
	}
	InnoLogger::Log(LogLevel::Verbose, "InnoJobScheduler: Reached synchronization point");
}

void InnoJobScheduler::AddJob(std::unique_ptr<IJob>&& job)
{
	m_JobPool.emplace(std::move(job));
}

void InnoJobScheduler::ExecuteJob(IJob* job, int threadID)
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
		std::uniform_int_distribution<> Dis(0, m_NumThreads - 1);
		l_ThreadIndex = Dis(Gen);
	}

	m_Threads[l_ThreadIndex]->AddJobToQueue(job);
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
		InnoJobScheduler::ExecuteJob(l_Jobs[i]);
	}

	while (FinishedJobCount != testCaseCount)
	{
	}

	InnoJobScheduler::WaitSync();

	auto l_Timestamp1 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

	auto l_JobTotalAsyncExecutionTime = l_Timestamp1 - l_StartTime;

	auto l_SpeedRatio = double(l_JobTotalAsyncExecutionTime) / double(JobTotalSyncExecutionTime);

	InnoLogger::Log(LogLevel::Success, "Async VS sync job execution speed ratio is ", l_SpeedRatio);

	InnoJobScheduler::Terminate();
}

void TestInnoRingBuffer(size_t testCaseCount)
{
	std::default_random_engine l_generator;
	std::uniform_int_distribution<unsigned int> l_randomSize(8, 16);
	std::uniform_int_distribution<unsigned int> l_randomOperation(16, 24);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		RingBuffer<float> l_ringBuffer;
		l_ringBuffer.reserve((size_t)std::pow(2, l_randomSize(l_generator)));
		auto l_testTime = l_ringBuffer.size() * 4;
		for (size_t j = 0; j < l_testTime; j++)
		{
			l_ringBuffer.emplace_back((float)j);
		}
	}
}

int main(int argc, char *argv[])
{
	TestIToA(8192);
	TestArray(8192);
	TestInnoMemory(65536);
	TestJob(512);
	TestInnoRingBuffer(128);
	return 0;
}