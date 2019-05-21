#include "MemorySystem.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

//Double-linked-list
class ChunkMarker
{
public:
	void* m_target = nullptr;
	ChunkMarker* m_next = nullptr;
	ChunkMarker* m_prev = nullptr;
};

class ObjectPool
{
public:
	ObjectPool() = delete;
	explicit ObjectPool(size_t objectSize, unsigned int poolCapability)
	{
		m_objectSize = objectSize;
		m_capability = poolCapability;
		m_poolSize = poolCapability * objectSize;
		m_poolPtr = ::new unsigned char[m_poolSize];
	};

	~ObjectPool() {
		::delete[] m_poolPtr;
	};

	size_t m_objectSize;
	unsigned long long m_capability = 0;
	unsigned long long m_poolSize = 0;
	unsigned char* m_poolPtr = nullptr;
};

class ObjectPoolInstance
{
public:
	ObjectPoolInstance() = delete;

	explicit ObjectPoolInstance(size_t objectSize, unsigned int poolCapability)
	{
		m_Pool = std::make_unique<ObjectPool>(objectSize, poolCapability);
		m_FreeChunkPool = std::make_unique<ObjectPool>(sizeof(ChunkMarker), poolCapability);
		m_CurrentFreeChunk = reinterpret_cast<ChunkMarker*>(m_FreeChunkPool->m_poolPtr);
	};

	~ObjectPoolInstance() = default;

	std::unique_ptr<ObjectPool> m_Pool;
	std::unique_ptr<ObjectPool> m_FreeChunkPool;
	ChunkMarker* m_CurrentFreeChunk;
};

class MemoryWatchdog
{
public:
	static MemoryWatchdog& get()
	{
		static MemoryWatchdog instance;

		return instance;
	}

	bool recordRawMemoryUsage(void* ptr, size_t size)
	{
		auto l_result = m_memo.find(ptr);
		if (l_result != m_memo.end())
		{
			auto l_ptrStr = InnoUtility::pointerToString(ptr);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: MemoryWatchdog: allocate collision happened at " + l_ptrStr + " !");
			return false;
		}
		else
		{
			m_memo.emplace(ptr, size);
			return true;
		}
	}

	bool removeRawMemoryUsage(void* ptr)
	{
		auto l_result = m_memo.find(ptr);
		if (l_result != m_memo.end())
		{
			m_memo.erase(ptr);
			return true;
		}
		else
		{
			auto l_ptrStr = InnoUtility::pointerToString(ptr);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: MemoryWatchdog: deallocate collision happened at " + l_ptrStr + " !");
			return false;
		}
	}

private:
	std::unordered_map<void*, size_t> m_memo;
};

INNO_PRIVATE_SCOPE InnoMemorySystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	std::vector<ObjectPoolInstance*> m_objectPoolInstances;

	bool fulfillChuckMarkerPool(ObjectPoolInstance* objectPoolInstance);

	bool setup();
}

bool InnoMemorySystemNS::fulfillChuckMarkerPool(ObjectPoolInstance* objectPoolInstance)
{
	auto l_chuckUC = objectPoolInstance->m_FreeChunkPool->m_poolPtr;
	auto l_componentUC = objectPoolInstance->m_Pool->m_poolPtr;

	ChunkMarker* l_prevFreeChunk = nullptr;

	/* walk through others */
	for (unsigned long long i = 0; i < objectPoolInstance->m_FreeChunkPool->m_capability; i++)
	{
		auto l_newFreeChunk = new(l_chuckUC) ChunkMarker();

		l_newFreeChunk->m_target = l_componentUC;
		l_newFreeChunk->m_prev = l_prevFreeChunk;

		// link from front to end
		if (l_prevFreeChunk)
		{
			l_newFreeChunk->m_prev->m_next = l_newFreeChunk;
		}

		l_newFreeChunk->m_next = nullptr;

		l_prevFreeChunk = l_newFreeChunk;

		l_chuckUC += sizeof(ChunkMarker);
		l_componentUC += objectPoolInstance->m_Pool->m_objectSize;
	}

	// initial free ChunkMarker
	objectPoolInstance->m_CurrentFreeChunk = reinterpret_cast<ChunkMarker*>(objectPoolInstance->m_FreeChunkPool->m_poolPtr);

	return true;
}

bool InnoMemorySystemNS::setup()
{
	InnoMemorySystemNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoMemorySystem::setup()
{
	return InnoMemorySystemNS::setup();
}

bool InnoMemorySystem::initialize()
{
	if (InnoMemorySystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoMemorySystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MemorySystem has been initialized.");
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Object is not created!");
		return false;
	}
}

bool InnoMemorySystem::update()
{
	if (InnoMemorySystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		InnoMemorySystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoMemorySystem::terminate()
{
	InnoMemorySystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MemorySystem has been terminated.");
	return true;
}
ObjectStatus InnoMemorySystem::getStatus()
{
	return InnoMemorySystemNS::m_objectStatus;
}

void* InnoMemorySystem::allocateMemoryPool(size_t objectSize, unsigned int poolCapability)
{
	auto l_objectPoolInstance = new ObjectPoolInstance(objectSize, poolCapability);

	InnoMemorySystemNS::fulfillChuckMarkerPool(l_objectPoolInstance);

	InnoMemorySystemNS::m_objectPoolInstances.emplace_back(l_objectPoolInstance);

	return l_objectPoolInstance;
}

void * InnoMemorySystem::allocateRawMemory(size_t size)
{
	auto m_Ptr = ::new char[size];
	MemoryWatchdog::get().recordRawMemoryUsage(m_Ptr, size);
	return m_Ptr;
}

bool InnoMemorySystem::deallocateRawMemory(void * ptr)
{
	delete[](char*)ptr;
	MemoryWatchdog::get().removeRawMemoryUsage(ptr);
	return true;
}

void * InnoMemorySystem::spawnObject(void * memoryPool, size_t objectSize)
{
	assert(memoryPool != nullptr);

	auto l_objectPoolInstance = reinterpret_cast<ObjectPoolInstance*>(memoryPool);

	assert(l_objectPoolInstance->m_Pool->m_objectSize == objectSize);

	if (!l_objectPoolInstance->m_CurrentFreeChunk)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Run out of memory pool!");
		return nullptr;
	}

	auto l_result = l_objectPoolInstance->m_CurrentFreeChunk->m_target;

	if (l_result)
	{
		// assign new free ChunkMarker
		auto l_next = l_objectPoolInstance->m_CurrentFreeChunk->m_next;
		if (l_next)
		{
			l_next->m_prev = nullptr;
			l_objectPoolInstance->m_CurrentFreeChunk = l_next;
		}
		else
		{
			l_objectPoolInstance->m_CurrentFreeChunk = nullptr;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "MemorySystem: Last free position has been allocated!");
		}
		return l_result;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Can't allocate memory!");
		return nullptr;
	}

	return nullptr;
}

bool InnoMemorySystem::destroyObject(void * memoryPool, size_t objectSize, void * object)
{
	assert(memoryPool != nullptr);

	auto l_objectPoolInstance = reinterpret_cast<ObjectPoolInstance*>(memoryPool);

	assert(l_objectPoolInstance->m_Pool->m_objectSize == objectSize);

	//get pointer distance between this object and the head of the pool
	auto l_offset = reinterpret_cast<unsigned char*>(object) - l_objectPoolInstance->m_Pool->m_poolPtr;

	auto l_index = l_offset / objectSize;

	//in-place allocate a ChunkMarker at the corresponding position
	auto l_freeChunkMarker = new(l_objectPoolInstance->m_FreeChunkPool->m_poolPtr + l_index * sizeof(ChunkMarker)) ChunkMarker();

	l_freeChunkMarker->m_target = object;

	//now insert after the current free chunk
	if (!l_objectPoolInstance->m_CurrentFreeChunk)
	{
		// edge case, last ChunkMarker
		l_freeChunkMarker->m_prev = nullptr;
		l_freeChunkMarker->m_next = nullptr;
		l_objectPoolInstance->m_CurrentFreeChunk = l_freeChunkMarker;
	}
	else
	{
		l_freeChunkMarker->m_prev = l_objectPoolInstance->m_CurrentFreeChunk;
		l_freeChunkMarker->m_next = l_objectPoolInstance->m_CurrentFreeChunk->m_next;
		l_objectPoolInstance->m_CurrentFreeChunk->m_next = l_freeChunkMarker;
	}

	//finally wipe away all the old data
	std::memset(object, 0, objectSize);

	return true;
}