#pragma once
#include "ObjectPool.h"
#include "ThreadSafeUnorderedMap.h"
#include "ThreadSafeVector.h"
#include "LogService.h"

namespace Inno
{
	template <typename T>
	class NamedObjectPool
	{
	public:
		NamedObjectPool() = default;

		void Initialize(uint32_t capacity)
		{
			m_Pool = TObjectPool<T>::Create(capacity);
		}

		void Terminate()
		{
			m_NameIndex.clear();
			m_LiveObjects.clear();
			TObjectPool<T>::Destruct(m_Pool);
			m_Pool = nullptr;
		}

		T* Allocate(const char* name)
		{
			if (!name || name[0] == '\0')
			{
				Log(Error, "NamedObjectPool: name cannot be empty.");
				return nullptr;
			}

			auto l_existing = m_NameIndex.find(name);
			if (l_existing != m_NameIndex.end())
				return l_existing->second;

			auto l_ptr = m_Pool->Spawn();
			if (!l_ptr)
			{
				Log(Error, "NamedObjectPool: pool exhausted for name: ", name);
				return nullptr;
			}

			l_ptr->m_ObjectStatus = ObjectStatus::Created;
			l_ptr->m_InstanceName = ObjectName(name);

			m_NameIndex.emplace(name, l_ptr);
			m_LiveObjects.emplace_back(l_ptr);
			return l_ptr;
		}

		void Release(T* ptr)
		{
			if (!ptr) return;
			m_NameIndex.erase(ptr->m_InstanceName.c_str());
			m_LiveObjects.eraseByValue(ptr);
			m_Pool->Destroy(ptr);
		}

		T* Find(const char* name)
		{
			auto l_result = m_NameIndex.find(name);
			return (l_result != m_NameIndex.end()) ? l_result->second : nullptr;
		}

		void ForEach(std::function<void(T*)> func)
		{
			m_LiveObjects.for_each(func);
		}

	private:
		TObjectPool<T>* m_Pool = nullptr;
		ThreadSafeUnorderedMap<std::string, T*> m_NameIndex;
		ThreadSafeVector<T*> m_LiveObjects;
	};
}
