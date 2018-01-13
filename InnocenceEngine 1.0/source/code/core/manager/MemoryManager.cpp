#include "../../main/stdafx.h"
#include "MemoryManager.h"

void MemoryManager::setup()
{
	this->setup(m_maxMemoryPoolSize);
}

void MemoryManager::setup(unsigned int memoryPoolSize)
{
	m_poolMemory = ::new unsigned char[memoryPoolSize];
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
}

inline void * MemoryManager::allocate(unsigned int size)
{
	return nullptr;
}

inline void MemoryManager::free(void * ptr)
{
}
