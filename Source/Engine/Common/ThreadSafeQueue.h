#pragma once
#include "STL14.h"
#include "STL17.h"

namespace Inno
{
    template <typename T>
	class ThreadSafeQueue
	{
	public:
		~ThreadSafeQueue(void)
		{
			invalidate();
		}

		bool tryPop(T &out)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			if (m_queue.empty() || !m_valid)
			{
				return false;
			}
			out = std::move(m_queue.front());
			m_queue.pop();
			return true;
		}

		bool waitPop(T &out)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_condition.wait(lock, [this]() {
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

		void push(const T &value)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue.push(value);
			m_condition.notify_one();
		}

		void push(T &&value)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue.push(std::move(value));
			m_condition.notify_one();
		}

		bool empty(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_queue.empty();
		}

		void clear(void)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			while (!m_queue.empty())
			{
				m_queue.pop();
			}
			m_condition.notify_all();
		}

		bool isValid(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_valid;
		}

		void invalidate(void)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_valid = false;
			m_condition.notify_all();
		}

		size_t size(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_queue.size();
		}

		// Run an inspector under the queue's shared lock against the head element
		// without popping. Returns true and invokes inspector(front()) when the
		// queue is non-empty and valid; returns false otherwise (inspector not
		// invoked). Inspector takes a const reference and must not retain it past
		// the call — the lock is released on return. Designed for cheap pointer
		// extraction (e.g. peekFront([&](const Task& t){ out = t.m_Component; }))
		// to avoid copying queue elements that hold std::vector or other heavy
		// payload, while keeping the existing tryPop / push / size API surface
		// unchanged.
		template <typename Inspector>
		bool peekFront(Inspector inspector) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			if (m_queue.empty() || !m_valid)
				return false;
			inspector(m_queue.front());
			return true;
		}

		std::queue<T> &getRawData(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_queue;
		}

		void setRawData(std::queue<T> &&values)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue = values;
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		std::queue<T> m_queue;
		std::condition_variable_any m_condition;
	};
}