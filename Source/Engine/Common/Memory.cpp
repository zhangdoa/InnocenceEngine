#include "Memory.h"
#include "LogService.h"
#include <memory>
#include <unordered_map>

#include "../Engine.h"

using namespace Inno;
;

void* Memory::Allocate(const std::size_t size)
{
	auto l_result = ::new char[size];

	if (g_Engine)
	{
		g_Engine->Get<Memory>()->Record(l_result, size);
	}
	
	return l_result;
}

void* Memory::Reallocate(void* const ptr, const std::size_t size)
{
	if (g_Engine)
	{
		g_Engine->Get<Memory>()->Erase(ptr);
		g_Engine->Get<Memory>()->Record(ptr, size);
	}
	
	auto l_result = realloc(ptr, size);
	return l_result;
}

void Memory::Deallocate(void* const ptr)
{
	if (g_Engine)
	{
		g_Engine->Get<Memory>()->Erase(ptr);
	}

	delete[](char*)ptr;
}

bool Memory::Record(void* ptr, std::size_t size)
{
	std::unique_lock<std::shared_mutex> lock{ m_Mutex };
	auto l_Result = m_Memo.find(ptr);
	if (l_Result != m_Memo.end())
	{
		Log(Warning, "Allocate collision happened at ", ptr, ".");
		return false;
	}
	else
	{
		m_Memo.emplace(ptr, size);
		return true;
	}
}

bool Memory::Erase(void* ptr)
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
		Log(Warning, "Deallocate collision happened at ", ptr, ".");
		return false;
	}
}