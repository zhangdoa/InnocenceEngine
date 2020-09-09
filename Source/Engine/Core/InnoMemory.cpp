#include "InnoMemory.h"
#include "InnoLogger.h"
#include <memory>
#include <unordered_map>

namespace MemoryMemo
{
	std::shared_mutex m_Mutex;
	std::unordered_map<void*, std::size_t> m_Memo;

	bool Record(void* ptr, std::size_t size)
	{
		std::unique_lock<std::shared_mutex> lock{ m_Mutex };
		auto l_Result = m_Memo.find(ptr);
		if (l_Result != m_Memo.end())
		{
			InnoLogger::Log(LogLevel::Warning, "InnoMemory: MemoryMemo: Allocate collision happened at ", ptr, ".");
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
		std::unique_lock<std::shared_mutex> lock{ m_Mutex };
		auto l_Result = m_Memo.find(ptr);
		if (l_Result != m_Memo.end())
		{
			m_Memo.erase(ptr);
			return true;
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "InnoMemory: MemoryMemo: Deallocate collision happened at ", ptr, ".");
			return false;
		}
	}
};

void* InnoMemory::Allocate(const std::size_t size)
{
	auto l_result = ::new char[size];
	MemoryMemo::Record(l_result, size);
	return l_result;
}

void* InnoMemory::Reallocate(void* const ptr, const std::size_t size)
{
	MemoryMemo::Erase(ptr);
	MemoryMemo::Record(ptr, size);
	auto l_result = realloc(ptr, size);
	return l_result;
}

void InnoMemory::Deallocate(void* const ptr)
{
	MemoryMemo::Erase(ptr);
	delete[](char*)ptr;
}