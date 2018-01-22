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
	memset(m_poolMemory, 0xCC, memoryPoolSize);
	m_freePoolSize = memoryPoolSize - sizeof(Chunk);
	m_totalPoolSize = memoryPoolSize;
	m_freePoolSize -= m_boundCheckSize * 2;

	// first free chuck
	Chunk freeChunk(memoryPoolSize - sizeof(Chunk) - m_boundCheckSize * 2);
	freeChunk.write(m_poolMemory + m_boundCheckSize);
	memcpy(m_poolMemory, m_startBound, m_boundCheckSize);
	memcpy(m_poolMemory + memoryPoolSize - m_boundCheckSize, m_endBound, m_boundCheckSize);
	LogManager::getInstance().printLog("Memory is allocated " + sizeof(m_poolMemory));
}

void MemoryManager::initialize()
{
	void* test = allocate(32);
	dumpToFile("memoryDump.innoDump");
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
	// add bound check size
	unsigned long l_requiredSize = size + sizeof(Chunk) + m_boundCheckSize * 2;
	
	// Now search for a block big enough, double linked list, O(n)
	Chunk* l_block = (Chunk*)(m_poolMemory + m_boundCheckSize);
	while (l_block)
	{
		if (l_block->m_free && l_block->m_chuckSize - l_requiredSize > m_minFreeBlockSize) { break; }
		l_block = l_block->m_next;
	}

	// If no block is found, return NULL
	if (!l_block) { return NULL; }

	// If the block is valid, create a new free block with 
	// what remains of the block memory

	unsigned char* l_blockData = (unsigned char*)l_block;

	unsigned long l_freeChuckSize = l_block->m_chuckSize - l_requiredSize;

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

	// update the pool size
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

inline void MemoryManager::dumpToFile(const std::string & fileName) const
{
	_iobuf* f = NULL;
	fopen_s(&f, fileName.c_str(), "w+");
	if (f)
	{
		fprintf(f, "Memory pool ----------------------------------\n");
		fprintf(f, "Type: Standard Memory\n");
		fprintf(f, "Total Size: %d\n", m_totalPoolSize);
		fprintf(f, "Free Size: %d\n", m_freePoolSize);

		// Now search for a block big enough
		Chunk* block = (Chunk*)(m_poolMemory + m_boundCheckSize);

		while (block)
		{
			if (block->m_free)
				fprintf(f, "Free:\t0x%08x [Bytes:%d]\n", block, block->m_chuckSize);
			else
				fprintf(f, "Used:\t0x%08x [Bytes:%d]\n", block, block->m_chuckSize);
			block = block->m_next;
		}

		fprintf(f, "\n\nMemory Dump:\n");
		unsigned char* ptr = m_poolMemory;
		unsigned char* charPtr = m_poolMemory;

		fprintf(f, "Start: 0x%08x\n", ptr);
		unsigned char i = 0;

		// Write the hex memory data
		unsigned long bytesPerLine = 4 * 4;

		fprintf(f, "\n0x%08x: ", ptr);
		fprintf(f, "%02x", *(ptr));
		++ptr;
		for (i = 1; ((unsigned long)(ptr - m_poolMemory) < m_totalPoolSize); ++i, ++ptr)
		{
			if (i == bytesPerLine)
			{
				// Write all the chars for this line now
				fprintf(f, "  ", charPtr);
				for (unsigned long charI = 0; charI<bytesPerLine; ++charI, ++charPtr)
					fprintf(f, "%c", *charPtr);
				charPtr = ptr;

				// Write the new line memory data
				fprintf(f, "\n0x%08x: ", ptr);
				fprintf(f, "%02x", *(ptr));
				i = 0;
			}
			else
				fprintf(f, ":%02x", *(ptr));
		}

		// Fill any gaps in the tab
		if ((unsigned long)(ptr - m_poolMemory) >= m_totalPoolSize)
		{
			unsigned long lastLineBytes = i;
			for (i; i< bytesPerLine; i++)
				fprintf(f, " --");

			// Write all the chars for this line now
			fprintf(f, "  ", charPtr);
			for (unsigned long charI = 0; charI<lastLineBytes; ++charI, ++charPtr)
				fprintf(f, "%c", *charPtr);
			charPtr = ptr;
		}
	}

	fclose(f);
}
