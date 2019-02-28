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

	auto isReady(void)
	{
		return m_future._Is_ready();
	}

private:
	std::future<T> m_future;
};