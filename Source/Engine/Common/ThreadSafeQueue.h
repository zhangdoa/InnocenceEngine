#pragma once
#include "STL14.h"
#include "Queue.h"

namespace Inno
{
	// Thread-safe wrapper over engine-native Inno::Queue<T>.
	template <typename T>
	class ThreadSafeQueue
	{
	public:
		~ThreadSafeQueue() { invalidate(); }

		bool tryPop(T& out)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			if (m_queue.empty() || !m_valid) return false;
			out = std::move(m_queue.front());
			m_queue.pop();
			return true;
		}

		bool waitPop(T& out)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_condition.wait(lock, [this]() { return !m_queue.empty() || !m_valid; });
			if (!m_valid) return false;
			out = std::move(m_queue.front());
			m_queue.pop();
			return true;
		}

		void push(const T& value)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue.push(value);
			m_condition.notify_one();
		}

		void push(T&& value)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue.push(std::move(value));
			m_condition.notify_one();
		}

		bool empty() const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_queue.empty();
		}

		size_t size() const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_queue.size();
		}

		void clear()
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue.clear();
			m_condition.notify_all();
		}

		bool isValid() const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_valid;
		}

		void invalidate()
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_valid = false;
			m_condition.notify_all();
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		Queue<T> m_queue;
		std::condition_variable_any m_condition;
	};
}
