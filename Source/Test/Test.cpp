#include "../Engine/Common/STL14.h"
#include "../Engine/Common/STL17.h"
#include "../Engine/Common/MathHelper.h"
#include "../Engine/Common/Timer.h"
#include "../Engine/Common/LogService.h"
#include "../Engine/Common/Memory.h"
#include "../Engine/Common/TaskScheduler.h"
#include "../Engine/Common/ObjectPool.h"
#include "../Engine/Common/Array.h"
#include "../Engine/Common/Atomic.h"
#include "../Engine/Common/Handle.h"
#include "../Engine/Common/Task.h"
#include "../Engine/Common/RingBuffer.h"

#include "../Engine/Engine.h"
using namespace Inno;

void TestIToA(size_t testCaseCount)
{
	int64_t int64 = std::numeric_limits<int64_t>::max();
	int32_t int32 = std::numeric_limits<int32_t>::max();

	auto l_StartTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_StringFromInt32 = ToString(int32);
		auto l_StringFromInt64 = ToString(int64);
	}

	auto l_Timestamp1 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_StringFromInt32 = std::to_string(int32);
		auto l_StringFromInt64 = std::to_string(int64);
	}

	auto l_Timestamp2 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio = double(l_Timestamp1 - l_StartTime) / double(l_Timestamp2 - l_Timestamp1);

	Log(Success, "Custom VS STL IToA speed ratio is ", l_SpeedRatio);
}

void TestArray(size_t testCaseCount)
{
	auto l_StartTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

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

	auto l_Timestamp1 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

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

	auto l_Timestamp2 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio = double(l_Timestamp1 - l_StartTime) / double(l_Timestamp2 - l_Timestamp1);

	Log(Success, "Custom VS STL array container speed ratio is ", l_SpeedRatio);
}

struct TestStruct
{
	uint32_t a[256];
};
void TestMemory(size_t testCaseCount)
{
	std::vector<uint32_t*> l_objectInCustomPool(testCaseCount);
	std::vector<void*> l_objectRaw(testCaseCount);

	auto l_objectPool = TObjectPool<uint32_t>::Create(testCaseCount);

	auto l_StartTime1 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_ptr = l_objectPool->Spawn();
		l_objectInCustomPool[i] = l_ptr;
	}

	auto l_Timestamp1 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_ptr = malloc(sizeof(TestStruct));
		l_objectRaw[i] = l_ptr;
	}

	auto l_Timestamp2 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio1 = double(l_Timestamp1 - l_StartTime1) / double(l_Timestamp2 - l_Timestamp1);

	Log(Success, "Custom object pool allocation VS malloc() speed ratio is ", l_SpeedRatio1);

	auto l_StartTime2 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_objectPool->Destroy(l_objectInCustomPool[i]);
	}

	auto l_Timestamp3 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		free(l_objectRaw[i]);
	}

	auto l_Timestamp4 = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio2 = double(l_Timestamp3 - l_StartTime2) / double(l_Timestamp4 - l_Timestamp3);

	Log(Success, "Custom object pool deallocation VS free() speed ratio is ", l_SpeedRatio1);

	TObjectPool<uint32_t>::Clear(l_objectPool);
	TObjectPool<uint32_t>::Destruct(l_objectPool);
}

template <typename T>
class AtomicDoubleBuffer
{
public:
	AtomicDoubleBuffer() = default;
	~AtomicDoubleBuffer() = default;

	void Initialize(const T& rhs)
	{
		{
			auto l_atomicWriter = AtomicWriter(m_A);
			*l_atomicWriter.Get() = rhs;
		}
		{
			auto l_atomicWriter = AtomicWriter(m_B);
			*l_atomicWriter.Get() = rhs;
		}
	}

	Atomic<T>& Get()
	{
		std::shared_lock<std::shared_mutex> lock{ m_Mutex };

		if (m_isANewer)
		{
			return m_A;
		}
		else
		{
			return m_B;
		}
	}

	void Flip()
	{
		m_isANewer = !m_isANewer;
	}

private:
	mutable std::shared_mutex m_Mutex;
	std::atomic_bool m_isANewer = true;
	Atomic<T> m_A;
	Atomic<T> m_B;
};

Atomic<uint32_t> l_atomicBuffer;
std::atomic<uint32_t> l_finishedTaskCount;

bool CheckCyclic(std::vector<Handle<ITask>> tasks, size_t initialIndex, size_t targetIndex)
{
	return false;
}

template <typename Func, typename... Args>
Handle<ITask> Submit(const char* name, int32_t threadID, Func&& func, Args&&... args)
{
	auto BoundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
	using ResultType = std::invoke_result_t<decltype(BoundTask)>;
	using PackagedTask = std::packaged_task<ResultType()>;
	using TaskType = Task<PackagedTask>;

	PackagedTask Task{ std::move(BoundTask) };
	auto l_task = std::make_unique<TaskType>(std::move(Task), name, ITask::Type::Once);
	auto l_handle = Handle<ITask>(l_task.get());
	g_Engine->Get<TaskScheduler>()->AddTask(l_handle, threadID);

	return l_handle;
}

void DispatchTestTasks(size_t testCaseCount, const std::function<void()>& job, bool buildTaskGraph = false)
{
	Log(Verbose, "Generate test async tasks...");

	std::vector<Handle<ITask>> l_Tasks;
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

	Log(Verbose, "Dispatch all tasks to async threads...");

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_Task = Submit(l_TaskNames[i].c_str(), -1, job);

		if (buildTaskGraph)
		{
			Handle<ITask> l_UpstreamTask;

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
		}

		l_Tasks.emplace_back(l_Task);
	}

	while (l_finishedTaskCount != testCaseCount)
	{
	}

	g_Engine->Get<TaskScheduler>()->Freeze();

	Log(Verbose, "All jobs finished.");
}

void TestRingBuffer(size_t testCaseCount)
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

void TestAtomic(size_t testCaseCount)
{
	std::function<void()> ExampleJob_Atomic = [&]()
	{
		std::default_random_engine l_generator;
		std::uniform_int_distribution<uint32_t> l_randomDelta(5, 10);

		auto l_executionTime = l_randomDelta(l_generator);

		{
			auto l_reader = AtomicReader(l_atomicBuffer);
			auto l_t = l_reader.Get();
			auto l_x = *l_t;
			Log(Warning, l_x);
		}

		{
			auto l_writer = AtomicWriter(l_atomicBuffer);
			auto l_t = l_writer.Get();
			*l_t += l_executionTime;
			std::this_thread::sleep_for(std::chrono::milliseconds(l_executionTime));
			Log(Success, *l_t);
		}

		{
			auto l_t = l_atomicBuffer;
			auto l_reader = AtomicReader(l_t);
			auto l_x = l_reader.Get();

			Log(Error, *l_x);
		}

		l_finishedTaskCount++;
	};

	l_finishedTaskCount = 0;
	DispatchTestTasks(testCaseCount, ExampleJob_Atomic);
}

void TestAtomicDoubleBuffer(size_t testCaseCount)
{
	const size_t l_testAtomicDoubleBufferSize = 256;
	std::array<float, 256> l_initData;
	l_initData.fill(0);

	AtomicDoubleBuffer<std::array<float, l_testAtomicDoubleBufferSize>> l_testAtomicDoubleBuffer;
	l_testAtomicDoubleBuffer.Initialize(l_initData);

	std::function<void()> ExampleJob_AtomicDoubleBuffer = [&]()
	{
		std::default_random_engine l_generator;
		std::uniform_int_distribution<uint32_t> l_randomDelta(0, 100);

		std::array<float, 256> l_writeData;

		{
			auto l_testAtomicDoubleBufferReader = AtomicReader(l_testAtomicDoubleBuffer.Get());
			l_writeData = *l_testAtomicDoubleBufferReader.Get();

			Log(Success, "Read l_testAtomicDoubleBuffer...");
		}

		auto l_executionTime = l_randomDelta(l_generator);
		std::this_thread::sleep_for(std::chrono::milliseconds(l_executionTime));

		if (l_executionTime > 50)
		{
			for (size_t i = 0; i < l_testAtomicDoubleBufferSize; i++)
			{
				l_writeData[i]++;
			}

			{
				auto l_testAtomicDoubleBufferWriter = AtomicWriter(l_testAtomicDoubleBuffer.Get());
				auto l_testAtomicDoubleBufferValue = l_testAtomicDoubleBufferWriter.Get();
				*l_testAtomicDoubleBufferValue = l_writeData;
				l_testAtomicDoubleBuffer.Flip();
			}

			Log(Warning, "Write l_testAtomicDoubleBuffer...");
		}

		l_finishedTaskCount++;
	};

	l_finishedTaskCount = 0;
	DispatchTestTasks(testCaseCount, ExampleJob_AtomicDoubleBuffer);

	auto l_testAtomicDoubleBufferReader = AtomicReader(l_testAtomicDoubleBuffer.Get());
	auto l_testAtomicDoubleBufferFinal = *l_testAtomicDoubleBufferReader.Get();

	for (size_t i = 0; i < l_testAtomicDoubleBufferSize; i++)
	{
		Log(Success, l_testAtomicDoubleBufferFinal[i]);
	}
}

class StackAllocator
{
public:
	StackAllocator() = delete;
	explicit StackAllocator(std::size_t size) noexcept
	{
		m_HeapAddress = reinterpret_cast<unsigned char*>(g_Engine->Get<Memory>()->Allocate(size));
		m_CurrentFreeChunk = m_HeapAddress;
	};

	~StackAllocator()
	{
		g_Engine->Get<Memory>()->Deallocate(m_HeapAddress);
	};

	void* Allocate(const std::size_t size);
	void Deallocate(void* const ptr);

private:
	unsigned char* m_HeapAddress;
	unsigned char* m_CurrentFreeChunk;
};

void* StackAllocator::Allocate(const std::size_t size)
{
	auto l_result = m_CurrentFreeChunk;
	m_CurrentFreeChunk += size;

	return l_result;
}

void StackAllocator::Deallocate(void* const ptr)
{
	m_CurrentFreeChunk = static_cast<unsigned char*>(ptr);
}

void TestStackAllocator(size_t testCaseCount)
{
	std::function<void()> ExampleJob_StackAllocator = [&]()
	{
		static thread_local StackAllocator l_stackAllocator(1024 * 1024);

		auto l_value = l_stackAllocator.Allocate(512);
		l_finishedTaskCount++;
	};

	l_finishedTaskCount = 0;
	DispatchTestTasks(testCaseCount, ExampleJob_StackAllocator);
}

int main(int argc, char* argv[])
{
	TestIToA(8192);
	TestArray(8192);
	TestMemory(65536);
	TestAtomic(128);
	TestAtomicDoubleBuffer(128);
	TestRingBuffer(128);
	TestStackAllocator(128);

	return 0;
}