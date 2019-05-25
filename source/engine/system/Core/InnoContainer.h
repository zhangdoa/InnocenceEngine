#include "InnoAllocator.h"

template<class T>
class InnoArray
{
public:
	InnoArray() = default;
	InnoArray(size_t elementCount)
	{
		m_ElementSize = sizeof(T);
		m_ElementCount = elementCount;
		m_HeapAddress = reinterpret_cast<T*>(InnoMemory::Allocate(m_ElementCount * m_ElementSize));
		m_CurrentFreeHeapAddress = m_HeapAddress;
	}

	InnoArray(const InnoArray<T> & rhs)
	{
		m_ElementSize = rhs.m_ElementSize;
		m_ElementCount = rhs.m_ElementCount;
		m_HeapAddress = reinterpret_cast<T*>(InnoMemory::Allocate(m_ElementCount * m_ElementSize));
		std::memcpy(rhs.m_HeapAddress, m_HeapAddress, m_ElementCount * m_ElementSize);
		m_CurrentFreeHeapAddress = rhs.m_CurrentFreeHeapAddress - rhs.m_HeapAddress + m_HeapAddress;
	}

	InnoArray<T>& operator=(const InnoArray<T> & rhs)
	{
		m_ElementSize = rhs.m_ElementSize;
		m_ElementCount = rhs.m_ElementCount;
		m_HeapAddress = reinterpret_cast<T*>(InnoMemory::Allocate(m_ElementCount * m_ElementSize));
		std::memcpy(m_HeapAddress, rhs.m_HeapAddress, m_ElementCount * m_ElementSize);
		m_CurrentFreeHeapAddress = rhs.m_CurrentFreeHeapAddress - rhs.m_HeapAddress + m_HeapAddress;
		return this;
	}

	InnoArray(InnoArray<T> && rhs)
	{
	}

	InnoArray<T>& operator=(InnoArray<T> && rhs)
	{
		return this;
	}

	T& operator[](size_t pos)
	{
		return *(m_HeapAddress + pos * m_ElementSize);
	}

	const T& operator[](size_t pos) const
	{
		return *(m_HeapAddress + pos * m_ElementSize);
	}

	auto Begin(void)
	{
		return m_HeapAddress;
	}

	auto End(void)
	{
		return m_HeapAddress + m_ElementCount * m_ElementSize;
	}

	auto Size(void)
	{
		return m_ElementCount;
	}

private:
	size_t m_ElementSize;
	size_t m_ElementCount;
	T* m_HeapAddress;
	T* m_CurrentFreeHeapAddress;
};
