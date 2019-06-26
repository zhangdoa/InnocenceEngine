#include "InnoMemory.h"
#include "InnoLogger.h"
#include <memory>
#include <unordered_map>

namespace InnoMemoryNS
{
	template<typename T>
	inline std::string PointerToString(T* ptr)
	{
		std::stringstream ss;
		ss << ptr;
		auto l_result = ss.str();
		return l_result;
	}
}

//Double-linked-list
struct Chunk
{
	void* m_Target = nullptr;
	Chunk* m_Next = nullptr;
	Chunk* m_Prev = nullptr;
};

class MemoryPool
{
public:
	MemoryPool() = delete;
	explicit MemoryPool(std::size_t objectSize, unsigned int capability) noexcept
	{
		m_ObjectSize = objectSize;
		m_Capability = capability;
		m_PoolSize = capability * objectSize;
		m_HeapAddress = reinterpret_cast<unsigned char*>(InnoMemory::Allocate(m_PoolSize));
	};

	~MemoryPool()
	{
		::delete[] m_HeapAddress;
	};

	unsigned char* GetHeapAddress() const
	{
		return m_HeapAddress;
	}

private:
	std::size_t m_ObjectSize = 0;
	unsigned long long m_Capability = 0;
	unsigned long long m_PoolSize = 0;
	unsigned char* m_HeapAddress = nullptr;
};

class ObjectPool : public IObjectPool
{
public:
	ObjectPool() = delete;

	explicit ObjectPool(std::size_t objectSize, unsigned int poolCapability)
	{
		m_Pool = std::make_unique<MemoryPool>(objectSize, poolCapability);
		m_ChunkPool = std::make_unique<MemoryPool>(sizeof(Chunk), poolCapability);
		m_CurrentFreeChunk = reinterpret_cast<Chunk*>(m_ChunkPool->GetHeapAddress());
		m_ObjectSize = objectSize;

		auto l_ChuckUC = m_ChunkPool->GetHeapAddress();
		auto l_ObjectUC = m_Pool->GetHeapAddress();
		Chunk* l_PrevFreeChunk = nullptr;

		for (unsigned long long i = 0; i < poolCapability; i++)
		{
			auto l_NewFreeChunk = new(l_ChuckUC) Chunk();

			l_NewFreeChunk->m_Target = l_ObjectUC;
			l_NewFreeChunk->m_Prev = l_PrevFreeChunk;

			// Link from front to end
			if (l_PrevFreeChunk)
			{
				l_NewFreeChunk->m_Prev->m_Next = l_NewFreeChunk;
			}

			l_NewFreeChunk->m_Next = nullptr;
			l_PrevFreeChunk = l_NewFreeChunk;

			l_ChuckUC += sizeof(Chunk);
			l_ObjectUC += objectSize;
		}

		auto l_PtrStr = InnoMemoryNS::PointerToString(this);
		InnoLogger::Log(LogLevel::Verbose, "InnoMemory: Object pool has been allocated at ", l_PtrStr.c_str(), ".");
	}

	~ObjectPool() = default;

	void* Spawn() override
	{
		if (!m_CurrentFreeChunk)
		{
			InnoLogger::Log(LogLevel::Error, "InnoMemory: Run out of object pool!");
			return nullptr;
		}

		auto l_Object = m_CurrentFreeChunk->m_Target;

		if (l_Object)
		{
			// Assign new free chunk
			auto l_Next = m_CurrentFreeChunk->m_Next;
			if (l_Next)
			{
				l_Next->m_Prev = nullptr;
				m_CurrentFreeChunk = l_Next;
			}
			else
			{
				m_CurrentFreeChunk = nullptr;
				InnoLogger::Log(LogLevel::Warning, "InnoMemory: Last free chuck has been allocated!");
			}
			return l_Object;
		}
		else
		{
			InnoLogger::Log(LogLevel::Error, "InnoMemory: Can't spawn object!");
			return nullptr;
		}
	}

	void Destroy(void* const ptr) override
	{
		//Get pointer distance between this object and the head of the pool
		auto l_Offset = reinterpret_cast<unsigned char*>(ptr) - m_Pool->GetHeapAddress();

		auto l_Index = l_Offset / m_ObjectSize;

		//Allocate in-place a Chunk at the corresponding position
		auto l_NewFreeChunk = new(m_ChunkPool->GetHeapAddress() + l_Index * sizeof(Chunk)) Chunk();

		l_NewFreeChunk->m_Target = ptr;

		//Insert after the current free chunk
		if (!m_CurrentFreeChunk)
		{
			// Edge case, last Chunk
			l_NewFreeChunk->m_Prev = nullptr;
			l_NewFreeChunk->m_Next = nullptr;
			m_CurrentFreeChunk = l_NewFreeChunk;
		}
		else
		{
			l_NewFreeChunk->m_Prev = m_CurrentFreeChunk;
			l_NewFreeChunk->m_Next = m_CurrentFreeChunk->m_Next;
			m_CurrentFreeChunk->m_Next = l_NewFreeChunk;
		}
	}

private:
	std::unique_ptr<MemoryPool> m_Pool;
	std::unique_ptr<MemoryPool> m_ChunkPool;
	Chunk* m_CurrentFreeChunk;
	std::size_t m_ObjectSize;
};

class MemoryMemo
{
public:
	static MemoryMemo& Get()
	{
		static MemoryMemo l_Instance;
		return l_Instance;
	}

	bool Record(void* ptr, std::size_t size)
	{
		auto l_Result = m_Memo.find(ptr);
		if (l_Result != m_Memo.end())
		{
			auto l_PtrStr = InnoMemoryNS::PointerToString(ptr);
			InnoLogger::Log(LogLevel::Error, "InnoMemory: MemoryMemo: Allocate collision happened at ", l_PtrStr.c_str(), " !");
			return false;
		}
		else
		{
			m_Memo.emplace(ptr, size);
			return true;
		}
	}

	bool Erase(void* ptr)
	{
		auto l_Result = m_Memo.find(ptr);
		if (l_Result != m_Memo.end())
		{
			m_Memo.erase(ptr);
			return true;
		}
		else
		{
			auto l_PtrStr = InnoMemoryNS::PointerToString(ptr);
			InnoLogger::Log(LogLevel::Error, "InnoMemory: MemoryMemo: Deallocate collision happened at ", l_PtrStr.c_str(), " !");
			return false;
		}
	}

private:
	std::unordered_map<void*, std::size_t> m_Memo;
};

void * InnoMemory::Allocate(const std::size_t size)
{
	auto m_Ptr = ::new char[size];
	MemoryMemo::Get().Record(m_Ptr, size);
	return m_Ptr;
}

void InnoMemory::Deallocate(void * const ptr)
{
	delete[](char*)ptr;
	MemoryMemo::Get().Erase(ptr);
}

IObjectPool * InnoMemory::CreateObjectPool(std::size_t objectSize, unsigned int poolCapability)
{
	auto l_IObjectPoolAddress = reinterpret_cast<IObjectPool*>(InnoMemory::Allocate(sizeof(ObjectPool)));
	auto l_IObjectPool = new(l_IObjectPoolAddress) ObjectPool(objectSize, poolCapability);
	return l_IObjectPool;
}