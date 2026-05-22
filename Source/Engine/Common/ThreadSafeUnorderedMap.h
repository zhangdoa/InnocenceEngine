#pragma once
#include "STL14.h"
#include "Allocator.h"

#include <unordered_map>

namespace Inno
{
	// Thread-safe map. Internal storage is std::unordered_map plumbed through
	// Inno::Allocator so allocations route through engine bookkeeping. A full
	// migration to Inno::HashMap is deferred — current consumers iterate via
	// std::unordered_map::iterator (.first/.second), which Inno::HashMap does
	// not yet expose.
	template <typename Key, typename T>
	class ThreadSafeUnorderedMap
	{
		using PairAlloc = Allocator<std::pair<const Key, T>>;
		using Map = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>, PairAlloc>;

	public:
		~ThreadSafeUnorderedMap() { invalidate(); }

		void reserve(std::size_t newCapacity)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_map.reserve(newCapacity);
			m_condition.notify_all();
		}

		void emplace(Key key, T value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_map.emplace(std::move(key), std::move(value));
			m_condition.notify_one();
		}

		void emplace(std::pair<Key, T> value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_map.emplace(std::move(value));
			m_condition.notify_one();
		}

		auto begin()       { std::shared_lock<std::shared_mutex> lock{m_mutex}; return m_map.begin(); }
		auto begin() const { std::shared_lock<std::shared_mutex> lock{m_mutex}; return m_map.begin(); }
		auto end()         { std::shared_lock<std::shared_mutex> lock{m_mutex}; return m_map.end(); }
		auto end()   const { std::shared_lock<std::shared_mutex> lock{m_mutex}; return m_map.end(); }

		auto find(const Key& key)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_map.find(key);
		}

		auto find(const Key& key) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_map.find(key);
		}

		auto erase(const Key& key)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			return m_map.erase(key);
		}

		template <typename PredicateT>
		void erase_if(const PredicateT& predicate)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			for (auto it = m_map.begin(); it != m_map.end();)
			{
				if (predicate(*it)) it = m_map.erase(it);
				else ++it;
			}
		}

		void clear()
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_map.clear();
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
			m_condition.notify_all();
		}

		size_t size() const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_map.size();
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		Map m_map;
		std::condition_variable_any m_condition;
	};
}
