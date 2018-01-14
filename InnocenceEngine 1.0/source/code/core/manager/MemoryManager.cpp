#include "../../main/stdafx.h"
#include "MemoryManager.h"

void MemoryManager::setup()
{
	this->setup(m_maxPoolSize);
}

void MemoryManager::setup(unsigned int memoryPoolSize)
{
	m_poolMemory = ::new unsigned char[memoryPoolSize];
	m_freePoolSize = memoryPoolSize - sizeof(Chunk);
	m_totalPoolSize = memoryPoolSize;
	m_freePoolSize -= sizeof(m_startBound) + sizeof(m_endBound);

	Chunk freeChunk(memoryPoolSize - sizeof(Chunk) - sizeof(m_startBound) - sizeof(m_endBound));
	freeChunk.write(m_poolMemory + sizeof(m_startBound));
	memcpy(m_poolMemory, m_startBound, sizeof(m_startBound));
	memcpy(m_poolMemory + memoryPoolSize - sizeof(m_startBound),
		m_endBound, sizeof(m_endBound));
	LogManager::getInstance().printLog("Memory is allocated " + sizeof(m_poolMemory));
}

void MemoryManager::initialize()
{
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

inline void * MemoryManager::allocate(unsigned int size)
{
	return nullptr;
}

inline void MemoryManager::free(void * ptr)
{
}
