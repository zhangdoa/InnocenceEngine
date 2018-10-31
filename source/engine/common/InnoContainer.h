#pragma once
#include "../common/stdafx.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

#include "InnoAllocator.h"

template <typename T>
class ThreadSafeQueue
{
public:
	~ThreadSafeQueue(void)
	{
		invalidate();
	}

	bool tryPop(T& out)
	{
		std::lock_guard<std::mutex> lock{ m_mutex };
		if (m_queue.empty() || !m_valid)
		{
			return false;
		}
		out = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}

	bool waitPop(T& out)
	{
		std::unique_lock<std::mutex> lock{ m_mutex };
		m_condition.wait(lock, [this]()
		{
			return !m_queue.empty() || !m_valid;
		});

		if (!m_valid)
		{
			return false;
		}
		out = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}

	void push(T value)
	{
		std::lock_guard<std::mutex> lock{ m_mutex };
		m_queue.push(std::move(value));
		m_condition.notify_one();
	}

	bool empty(void) const
	{
		std::lock_guard<std::mutex> lock{ m_mutex };
		return m_queue.empty();
	}

	void clear(void)
	{
		std::lock_guard<std::mutex> lock{ m_mutex };
		while (!m_queue.empty())
		{
			m_queue.pop();
		}
		m_condition.notify_all();
	}

	bool isValid(void) const
	{
		std::lock_guard<std::mutex> lock{ m_mutex };
		return m_valid;
	}

	void invalidate(void)
	{
		std::lock_guard<std::mutex> lock{ m_mutex };
		m_valid = false;
		m_condition.notify_all();
	}

	size_t size(void)
	{
		std::lock_guard<std::mutex> lock{ m_mutex };
		return m_queue.size();
	}

private:
	std::atomic_bool m_valid{ true };
	mutable std::mutex m_mutex;
	std::queue<T> m_queue;
	std::condition_variable m_condition;
};

#ifdef INNO_PLATFORM_WIN64
template<class _Ty, class _Ax = innoAllocator<_Ty> >
class innoList : public std::list<_Ty, _Ax>
{
};

template<class _Ty, class _Ax = innoAllocator<_Ty> >
class innoVector : public std::vector<_Ty, _Ax>
{
};
#endif
