#pragma once
#include <future>

#include "InnoContainer.h"


class IThreadTask
{
public:
	IThreadTask(void) = default;
	virtual ~IThreadTask(void) = default;
	IThreadTask(const IThreadTask& rhs) = delete;
	IThreadTask& operator=(const IThreadTask& rhs) = delete;
	IThreadTask(IThreadTask&& other) = default;
	IThreadTask& operator=(IThreadTask&& other) = default;

	virtual void execute() = 0;
};

template <typename Func>
class InnoTask : public IThreadTask
{
public:
	InnoTask(Func&& func)
		:m_func{ std::move(func) }
	{
	}

	~InnoTask(void) override = default;
	InnoTask(const InnoTask& rhs) = delete;
	InnoTask& operator=(const InnoTask& rhs) = delete;
	InnoTask(InnoTask&& other) = default;
	InnoTask& operator=(InnoTask&& other) = default;

	void execute() override
	{
		m_func();
	}

private:
	Func m_func;
};

template <typename T>
class InnoFuture
{
public:
	InnoFuture(std::future<T>&& future)
		:m_future{ std::move(future) }
	{
	}

	InnoFuture(const InnoFuture& rhs) = delete;
	InnoFuture& operator=(const InnoFuture& rhs) = delete;
	InnoFuture(InnoFuture&& other) = default;
	InnoFuture& operator=(InnoFuture&& other) = default;

	~InnoFuture(void)
	{
		if (m_future.valid())
		{
			m_future.get();
		}
	}

	auto get(void)
	{
		return m_future.get();
	}

private:
	std::future<T> m_future;
};

class InnoThreadPool
{
public:
	InnoThreadPool(void)
		:InnoThreadPool{ std::max<unsigned int>(std::thread::hardware_concurrency(), 2u) - 1u }
	{
	}

	explicit InnoThreadPool(const std::uint32_t numThreads)
		:m_done{ false },
		m_workQueue{},
		m_threads{}
	{
		try
		{
			for (std::uint32_t i = 0u; i < numThreads; ++i)
			{
				m_threads.emplace_back(&InnoThreadPool::worker, this);
			}
		}
		catch (...)
		{
			destroy();
			throw;
		}
	}

	InnoThreadPool(const InnoThreadPool& rhs) = delete;
	InnoThreadPool& operator=(const InnoThreadPool& rhs) = delete;

	~InnoThreadPool(void)
	{
		destroy();
	}

	template <typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)
	{
		auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
		using ResultType = std::result_of_t<decltype(boundTask)()>;
		using PackagedTask = std::packaged_task<ResultType()>;
		using TaskType = InnoTask<PackagedTask>;

		PackagedTask task{ std::move(boundTask) };
		InnoFuture<ResultType> result{ task.get_future() };
		m_workQueue.push(std::make_unique<TaskType>(std::move(task)));
		return result;
	}

private:
	void worker(void)
	{
		while (!m_done)
		{
			std::unique_ptr<IThreadTask> pTask{ nullptr };
			if (m_workQueue.waitPop(pTask))
			{
				pTask->execute();
			}
		}
	}
	void destroy(void)
	{
		m_done = true;
		m_workQueue.invalidate();
		for (auto& thread : m_threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}

private:
	std::atomic_bool m_done;
	ThreadSafeQueue<std::unique_ptr<IThreadTask>> m_workQueue;
	std::vector<std::thread> m_threads;
};
