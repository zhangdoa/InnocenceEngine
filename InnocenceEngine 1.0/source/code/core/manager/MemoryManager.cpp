#include "../../main/stdafx.h"
#include "MemoryManager.h"

void MemoryManager::setup()
{
	this->setup(m_maxPoolSize);
}

void MemoryManager::setup(unsigned long  memoryPoolSize)
{
	// Allocate memory pool
	m_poolMemory = ::new unsigned char[memoryPoolSize];
	m_freePoolSize = memoryPoolSize - sizeof(Chunk);
	m_totalPoolSize = memoryPoolSize;
	m_freePoolSize -= m_boundCheckSize * 2;

	// first free chuck
	Chunk freeChunk(memoryPoolSize - sizeof(Chunk) - m_boundCheckSize * 2);
	freeChunk.write(m_poolMemory + m_boundCheckSize);
	memcpy(m_poolMemory, m_startBound, m_boundCheckSize);
	memcpy(m_poolMemory + memoryPoolSize - m_boundCheckSize,
		m_endBound, m_boundCheckSize);
	LogManager::getInstance().printLog("Memory is allocated " + sizeof(m_poolMemory));
}

void MemoryManager::initialize()
{
	void* test = allocate(16);
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("MemoryManager has been initialized.");
}

void MemoryManager::update()
{
}

void MemoryManager::shutdown()
{
	::delete[] m_poolMemory;
	LogManager::getInstance().printLog("MemoryManager has been shutdown.");
}

inline void * MemoryManager::allocate(unsigned long  size)
{
	unsigned long l_requiredSize = size + sizeof(Chunk);
	
	// add bound check size
	l_requiredSize += m_boundCheckSize * 2;

	// Now search for a block big enough, double linked list, O(n)
	Chunk* l_block = (Chunk*)(m_poolMemory + m_boundCheckSize);
	while (l_block)
	{
		if (l_block->m_free && l_block->m_chuckSize >= l_requiredSize) { break; }
		l_block = l_block->m_next;
	}

	unsigned char* l_blockData = (unsigned char*)l_block;

	// If no block is found, return NULL
	if (!l_block) { return NULL; }

	// If the block is valid, create a new free block with 
	// what remains of the block memory
	unsigned long l_freeChuckSize = l_block->m_chuckSize - l_requiredSize;
	if (l_freeChuckSize > m_minFreeBlockSize)
	{
		Chunk freeBlock(l_freeChuckSize);
		freeBlock.m_next = l_block->m_next;
		freeBlock.m_prev = l_block;
		freeBlock.write(l_blockData + l_requiredSize);
		if (freeBlock.m_next)
		{
			freeBlock.m_next->m_prev = (Chunk*)(l_blockData + l_requiredSize);
			memcpy(l_blockData + l_requiredSize - m_boundCheckSize, m_startBound,
				m_boundCheckSize);
			l_block->m_next = (Chunk*)(l_blockData + l_requiredSize);
			l_block->m_chuckSize = size;
		}
	}

	// If a block is found, update the pool size
	m_freePoolSize -= l_block->m_chuckSize;

	// Set the memory block
	l_block->m_free = false;

	// Move the memory around
	memcpy(l_blockData - m_boundCheckSize, m_startBound, m_boundCheckSize);
	memcpy(l_blockData + sizeof(Chunk) + l_block->m_chuckSize, m_endBound,
		m_boundCheckSize);

	return (l_blockData + sizeof(Chunk));
}

inline void MemoryManager::free(void * ptr)
{
}
