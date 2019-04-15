#include "InnoAllocator.h"
#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

void innoHeapAllocator::deallocate(void * const ptr)
{
	g_pCoreSystem->getMemorySystem()->deallocateRawMemory(ptr);
}

void* innoHeapAllocator::allocate(const size_t size)
{
	return g_pCoreSystem->getMemorySystem()->allocateRawMemory(size);
}