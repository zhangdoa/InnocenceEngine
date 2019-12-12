#include "../Engine/Common/InnoContainer.h"
#include "../Engine/Common/InnoMathHelper.h"
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
	static std::vector<uint32_t> l_objectSizes = { 64, 128, 256, 512, 1024, 2048, 4096, 8192 };
	std::default_random_engine l_generator;
	std::uniform_int_distribution<uint32_t> l_randomDelta(0, 7);
	auto l_objectSize = l_objectSizes[l_randomDelta(l_generator)];

	std::vector<void*> l_objectInCustomPool(testCaseCount);
	std::vector<void*> l_objectRaw(testCaseCount);

	auto l_objectPool = InnoMemory::CreateObjectPool(l_objectSize, (uint32_t)testCaseCount);

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

	InnoMemory::ClearObjectPool(l_objectPool);
	InnoMemory::DestroyObjectPool(l_objectPool);
}

template <typename T>
class AtomicDoubleBuffer
{
public:
	AtomicDoubleBuffer() = default;
	~AtomicDoubleBuffer() = default;

	Atomic<T>& Read()
	{
		std::shared_lock<std::shared_mutex> lock{ m_Mutex };

		if (m_isBNewer)
		{
			return m_B;
		}
		else
		{
			return m_A;
		}
	}

	void Write(T&& value)
	{
		std::unique_lock<std::shared_mutex> lock{ m_Mutex };

		if (m_isBNewer)
		{
			auto l_writer = AtomicWriter(m_A);
			auto l_lhs = l_writer.Get();
			l_lhs = std::move(value);
			m_isBNewer = false;
		}
		else
		{
			auto l_writer = AtomicWriter(m_B);
			auto l_lhs = l_writer.Get();
			l_lhs = std::move(value);
			m_isBNewer = true;
		}
	}

	void Reserve(size_t elementCount)
	{
		std::unique_lock<std::shared_mutex> lock{ m_Mutex };

		auto l_writerA = AtomicWriter(m_A);
		auto l_lhsA = l_writerA.Get();
		auto l_writerB = AtomicWriter(m_B);
		auto l_lhsB = l_writerB.Get();

		l_lhsA->reserve(elementCount);
		l_lhsB->reserve(elementCount);
	}

private:
	mutable std::shared_mutex m_Mutex;
	std::atomic_bool m_isBNewer = false;
	Atomic<T> m_A;
	Atomic<T> m_B;
};

Atomic<uint32_t> l_atomicBuffer;
std::atomic<uint32_t> l_finishedTaskCount;
std::default_random_engine l_generator;
std::uniform_int_distribution<uint32_t> l_randomDelta(5, 10);

bool CheckCyclic(std::vector<std::shared_ptr<IInnoTask>> tasks, size_t initialIndex, size_t targetIndex)
{
	auto l_currentTask = tasks[initialIndex];
	auto l_targetTask = tasks[targetIndex];

	bool l_hasCyclic;

	while (l_targetTask)
	{
		l_hasCyclic = (l_currentTask == l_targetTask);

		if (l_hasCyclic)
		{
			break;
		}

		l_targetTask = l_targetTask->GetUpstreamTask();
		if (!l_targetTask)
		{
			l_hasCyclic = false;
			break;
		}
	}

	return l_hasCyclic;
}

template <typename Func, typename... Args>
std::shared_ptr<IInnoTask> submit(const char* name, int32_t threadID, const std::shared_ptr<IInnoTask>& upstreamTask, Func&& func, Args&&... args)
{
	auto BoundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
	using ResultType = std::invoke_result_t<decltype(BoundTask)>;
	using PackagedTask = std::packaged_task<ResultType()>;
	using TaskType = InnoTask<PackagedTask>;

	PackagedTask Task{ std::move(BoundTask) };
	auto l_task = std::make_unique<TaskType>(std::move(Task), name, upstreamTask);
	return InnoTaskScheduler::AddTaskImpl(std::move(l_task), threadID);
}

void TestTaskScheduler(size_t testCaseCount, const std::function<void()>& job)
{
	InnoLogger::Log(LogLevel::Verbose, "Generate test async tasks...");

	std::vector<std::shared_ptr<IInnoTask>> l_Tasks;
	std::vector<std::string> l_TaskNames;
	l_Tasks.reserve(testCaseCount);
	l_TaskNames.reserve(testCaseCount);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_TaskName = std::string("TestTask_" + std::to_string(i) + "/");
		l_TaskNames.emplace_back(l_TaskName);
	}

	// We need a DAG structure
	std::default_random_engine l_generator;

	InnoLogger::Log(LogLevel::Verbose, "Dispatch all tasks to async threads...");

	for (size_t i = 0; i < testCaseCount; i++)
	{
		std::shared_ptr<IInnoTask> l_UpstreamTask;

		if (i > 1)
		{
			std::uniform_int_distribution<uint32_t> l_randomDelta(0, (uint32_t)i - 1);

			size_t l_UpstreamTaskIndex = l_randomDelta(l_generator);

			// Eliminate cyclic
			while (CheckCyclic(l_Tasks, i - 1, l_UpstreamTaskIndex))
			{
				l_UpstreamTaskIndex = l_randomDelta(l_generator);
			}
			l_UpstreamTask = l_Tasks[l_UpstreamTaskIndex];
		}

		auto l_Task = submit(l_TaskNames[i].c_str(), -1, l_UpstreamTask, job);

		l_Tasks.emplace_back(l_Task);
	}

	while (l_finishedTaskCount != testCaseCount)
	{
	}

	InnoTaskScheduler::WaitSync();

	InnoLogger::Log(LogLevel::Verbose, "All jobs finished.");
}

void TestInnoRingBuffer(size_t testCaseCount)
{
	std::default_random_engine l_generator;
	std::uniform_int_distribution<uint32_t> l_randomSize(8, 16);
	std::uniform_int_distribution<uint32_t> l_randomOperation(16, 24);

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
	InnoTaskScheduler::Setup();
	InnoTaskScheduler::Initialize();

	TestIToA(8192);
	TestArray(8192);
	TestInnoMemory(65536);

	std::function<void()> ExampleJob_Atomic = [&]()
	{
		auto l_executionTime = l_randomDelta(l_generator);

		{
			auto l_reader = AtomicReader(l_atomicBuffer);
			auto l_t = l_reader.Get();
			auto l_x = *l_t;
			InnoLogger::Log(LogLevel::Warning, l_x);
		}

		{
			auto l_writer = AtomicWriter(l_atomicBuffer);
			auto l_t = l_writer.Get();
			*l_t += l_executionTime;
			std::this_thread::sleep_for(std::chrono::milliseconds(l_executionTime));
			InnoLogger::Log(LogLevel::Success, *l_t);
		}

		{
			auto l_t = l_atomicBuffer;
			auto l_reader = AtomicReader(l_t);
			auto l_x = l_reader.Get();

			InnoLogger::Log(LogLevel::Error, *l_x);
		}

		l_finishedTaskCount++;
	};

	std::function<void()> ExampleJob_AtomicDoubleBuffer = [&]()
	{
		AtomicDoubleBuffer<std::vector<float>> l_test;

		l_test.Reserve(1024);

		auto l_testData = l_test.Read();

		l_finishedTaskCount++;
	};

	l_finishedTaskCount = 0;
	TestTaskScheduler(128, ExampleJob_Atomic);
	l_finishedTaskCount = 0;
	TestTaskScheduler(128, ExampleJob_AtomicDoubleBuffer);

	TestInnoRingBuffer(128);
	InnoTaskScheduler::Terminate();

	return 0;
}