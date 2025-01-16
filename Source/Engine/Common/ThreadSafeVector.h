#pragma once
#include "STL14.h"

namespace Inno
{
	template <typename T>
	class ThreadSafeVector
	{
	public:
		ThreadSafeVector()
		{
		}

		ThreadSafeVector(const ThreadSafeVector &rhs)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = rhs.m_vector;
		}
		ThreadSafeVector &operator=(const ThreadSafeVector &rhs)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = rhs.m_vector;
			return this;
		}
		ThreadSafeVector(ThreadSafeVector &&rhs)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = std::move(rhs.m_vector);
		}
		ThreadSafeVector &operator=(ThreadSafeVector &&rhs)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = std::move(rhs.m_vector);
			return this;
		}

		~ThreadSafeVector(void)
		{
			invalidate();
		}

		T &operator[](std::size_t pos)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector[pos];
		}

		const T &operator[](std::size_t pos) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector[pos];
		}

		void reserve(const std::size_t _Newcapacity)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.reserve(_Newcapacity);
		}

		void push_back(T &&value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.push_back(value);
		}

		template <class... Args>
		void emplace_back(Args &&... values)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.emplace_back(values...);
		}

		auto empty(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.empty();
		}

		void clear(void)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.clear();
		}

		auto begin(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.begin();
		}

		auto begin(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.begin();
		}

		auto end(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.end();
		}

		auto end(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.end();
		}

		void eraseByIndex(size_t index)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.erase(index);
		}

		void eraseByValue(const T &value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.erase(std::remove(std::begin(m_vector), std::end(m_vector), value), std::end(m_vector));
		}

		typename std::vector<T>::iterator erase(typename std::vector<T>::const_iterator _First, typename std::vector<T>::const_iterator _Last)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.erase(_First, _Last);
		}

		auto size(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.size();
		}

		void shrink_to_fit()
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.shrink_to_fit();
		}

		bool isValid(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_valid;
		}

		void invalidate(void)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_valid = false;
		}

		std::vector<T> &getRawData(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector;
		}

		void setRawData(std::vector<T> &&values)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = values;
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		std::vector<T> m_vector;
	};
    }