#pragma once
#include "STL14.h"

namespace Inno
{
	template <typename Key, typename T>
	class ThreadSafeUnorderedMap
	{
	public:
		~ThreadSafeUnorderedMap(void)
		{
			invalidate();
		}

		void reserve(const std::size_t _Newcapacity)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_unordered_map.reserve(_Newcapacity);
			m_condition.notify_all();
		}

		void emplace(Key key, T value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_unordered_map.emplace(key, value);
			m_condition.notify_one();
		}

		void emplace(std::pair<Key, T> value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_unordered_map.emplace(value);
			m_condition.notify_one();
		}

		auto begin(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.begin();
		}

		auto begin(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.begin();
		}

		auto end(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.end();
		}

		auto end(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.end();
		}

		auto find(const Key &key)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.find(key);
		}

		auto find(const Key &key) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.find(key);
		}

		auto erase(const Key &key)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.erase(key);
		}

		template <typename PredicateT>
		void erase_if(const PredicateT &predicate)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			for (auto it = m_unordered_map.begin(); it != m_unordered_map.end();)
			{
				if (predicate(*it))
				{
					it = m_unordered_map.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		auto clear(void)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.clear();
		}

		void invalidate(void)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_valid = false;
			m_condition.notify_all();
		}

		auto size(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.size();
		}

		std::unordered_map<Key, T> &getRawData(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map;
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		std::unordered_map<Key, T> m_unordered_map;
		std::condition_variable_any m_condition;
	};
}