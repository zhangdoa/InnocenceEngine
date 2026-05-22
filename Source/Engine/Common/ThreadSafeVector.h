#pragma once
#include "STL14.h"
#include "STL17.h"
#include "Array.h"

namespace Inno
{
	// Thread-safe wrapper over engine-native Inno::Array<T>.
	template <typename T>
	class ThreadSafeVector
	{
	public:
		ThreadSafeVector() = default;

		ThreadSafeVector(const ThreadSafeVector& rhs)
		{
			std::shared_lock<std::shared_mutex> lock{rhs.m_mutex};
			m_array = rhs.m_array;
		}

		ThreadSafeVector& operator=(const ThreadSafeVector& rhs)
		{
			if (this != &rhs)
			{
				std::shared_lock<std::shared_mutex> r{rhs.m_mutex};
				std::unique_lock<std::shared_mutex> w{m_mutex};
				m_array = rhs.m_array;
			}
			return *this;
		}

		ThreadSafeVector(ThreadSafeVector&& rhs) noexcept
		{
			std::unique_lock<std::shared_mutex> lock{rhs.m_mutex};
			m_array = std::move(rhs.m_array);
		}

		ThreadSafeVector& operator=(ThreadSafeVector&& rhs) noexcept
		{
			if (this != &rhs)
			{
				std::unique_lock<std::shared_mutex> r{rhs.m_mutex};
				std::unique_lock<std::shared_mutex> w{m_mutex};
				m_array = std::move(rhs.m_array);
			}
			return *this;
		}

		~ThreadSafeVector() { invalidate(); }

		T operator[](std::size_t pos) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_array[pos];
		}

		void reserve(std::size_t n)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_array.reserve(n);
		}

		void push_back(const T& value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_array.push_back(value);
		}

		void push_back(T&& value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_array.push_back(std::move(value));
		}

		template <class... Args>
		void emplace_back(Args&&... args)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_array.emplace_back(std::forward<Args>(args)...);
		}

		bool empty() const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_array.empty();
		}

		void clear()
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_array.clear();
		}

		size_t size() const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_array.size();
		}

		void shrink_to_fit()
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_array.shrink_to_fit();
		}

		void eraseByValue(const T& value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			size_t out = 0;
			for (size_t i = 0; i < m_array.size(); ++i)
			{
				if (!(m_array[i] == value))
				{
					if (out != i) m_array[out] = std::move(m_array[i]);
					++out;
				}
			}
			while (m_array.size() > out) m_array.pop_back();
		}

		template <typename Pred>
		void erase_if(Pred&& pred)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			size_t out = 0;
			for (size_t i = 0; i < m_array.size(); ++i)
			{
				if (!pred(m_array[i]))
				{
					if (out != i) m_array[out] = std::move(m_array[i]);
					++out;
				}
			}
			while (m_array.size() > out) m_array.pop_back();
		}

		template <typename Func>
		void for_each(Func&& func) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			for (size_t i = 0; i < m_array.size(); ++i) func(m_array[i]);
		}

		template <typename Func>
		void for_each(Func&& func)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			for (size_t i = 0; i < m_array.size(); ++i) func(m_array[i]);
		}

		bool isValid() const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_valid;
		}

		void invalidate()
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_valid = false;
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		Array<T> m_array;
	};
}
