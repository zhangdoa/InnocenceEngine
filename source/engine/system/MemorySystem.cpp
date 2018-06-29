#include "MemorySystem.h"

void MemorySystem::setup()
{
	this->setup(m_maxPoolSize);
}

void MemorySystem::setup(unsigned long  memoryPoolSize)
{
	// Allocate memory pool
	m_poolMemory = nullptr;
	m_poolMemory = ::new unsigned char[memoryPoolSize];
	memset(m_poolMemory, 0xCC, memoryPoolSize);
	m_freePoolSize = memoryPoolSize - sizeof(Chunk);
	m_totalPoolSize = memoryPoolSize;
	m_freePoolSize -= m_boundCheckSize * 2;

	// first free chuck
	Chunk freeChunk(memoryPoolSize - sizeof(Chunk) - m_boundCheckSize * 2);
	freeChunk.write(m_poolMemory + m_boundCheckSize);
	std::memcpy(m_poolMemory, m_startBound, m_boundCheckSize);
	std::memcpy(m_poolMemory + memoryPoolSize - m_boundCheckSize, m_endBound, m_boundCheckSize);
}

void MemorySystem::initialize()
{
	m_objectStatus = objectStatus::ALIVE;
	g_pLogSystem->printLog("MemorySystem has been initialized.");
}

void MemorySystem::update()
{
}

void MemorySystem::shutdown()
{
	::delete[] m_poolMemory;
}

void * MemorySystem::allocate(unsigned long size)
{
	// add bound check size
	unsigned long l_requiredSize = size + sizeof(Chunk) + m_boundCheckSize * 2;
	// alignment to 4
	for (size_t i = 0; i < s_NumBlockSizes; i++)
	{
		if (l_requiredSize < s_BlockSizes[i])
		{
			l_requiredSize = s_BlockSizes[i];
			break;
		}
	}
	// Now search for a block big enough, double linked list, O(n)
	Chunk* l_block = reinterpret_cast<Chunk*>(m_poolMemory + m_boundCheckSize);
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
		freeBlock.m_next->m_prev = reinterpret_cast<Chunk*>(l_blockData + l_requiredSize);
	}

	std::memcpy(l_blockData + l_requiredSize - m_boundCheckSize, m_startBound,
		m_boundCheckSize);
	l_block->m_next = reinterpret_cast<Chunk*>(l_blockData + l_requiredSize);
	l_block->m_chuckSize = size;

	// update the pool size
	m_freePoolSize -= l_block->m_chuckSize;

	// Set the memory block
	l_block->m_free = false;

	// Move the memory around
	std::memcpy(l_blockData - m_boundCheckSize, m_startBound, m_boundCheckSize);
	std::memcpy(l_blockData + sizeof(Chunk) + l_block->m_chuckSize, m_endBound,
		m_boundCheckSize);

	std::memset(l_blockData + sizeof(Chunk), 0xAB,
		l_block->m_chuckSize);

	return (l_blockData + sizeof(Chunk));
}

void MemorySystem::free(void * ptr)
{
	// is a valid node?
	if (!ptr) return;
	Chunk* block = reinterpret_cast<Chunk*>((unsigned char*)ptr - sizeof(Chunk));
	if (block->m_free) return;

	unsigned long l_fullBlockSize = block->m_chuckSize + sizeof(Chunk) + m_boundCheckSize * 2;
	m_freePoolSize += block->m_chuckSize;

	Chunk* headBlock = block;
	Chunk* prev = block->m_prev;
	Chunk* next = block->m_next;

	// If the node before is free I merge it with this one
	if (block->m_prev && block->m_prev->m_free)
	{
		headBlock = block->m_prev;
		prev = block->m_prev->m_prev;
		next = block->m_next;

		// Include the prev node in the block size so we trash it as well
		l_fullBlockSize += block->m_prev->m_chuckSize + sizeof(Chunk) + m_boundCheckSize * 2;

		// If there is a next one, we need to update its pointer
		if (block->m_next)
		{
			// We will re point the next
			block->m_next->m_prev = headBlock;

			// Include the next node in the block size if it is
			// free so we trash it as well
			if (block->m_next->m_free)
			{
				// We will point to next's next
				next = block->m_next->m_next;
				if (block->m_next->m_next)
				{
					block->m_next->m_next->m_prev = headBlock;
				}
				l_fullBlockSize += block->m_next->m_chuckSize + sizeof(Chunk) + m_boundCheckSize * 2;
			}
		}
	}
	else
		// If next node is free lets merge it to the current one
		if (block->m_next && block->m_next->m_free)
		{
			headBlock = block;
			prev = block->m_prev;
			next = block->m_next->m_next;

			// Include the next node in the block size so we trash it as well
			l_fullBlockSize += block->m_next->m_chuckSize + sizeof(Chunk) + m_boundCheckSize * 2;
		}
	// Create the free block
	unsigned char* freeBlockStart = (unsigned char*)headBlock;
	memset(freeBlockStart - m_boundCheckSize, 0xCC, l_fullBlockSize);

	unsigned long l_freeUserDataSize = l_fullBlockSize - sizeof(Chunk);
	l_freeUserDataSize = l_freeUserDataSize - m_boundCheckSize * 2;

	Chunk freeBlock(l_freeUserDataSize);
	freeBlock.m_prev = prev;
	freeBlock.m_next = next;
	freeBlock.write(freeBlockStart);

	// Move the memory around if guards are needed
	std::memcpy(freeBlockStart - m_boundCheckSize, m_startBound, m_boundCheckSize);
	std::memcpy(freeBlockStart + sizeof(Chunk) + l_freeUserDataSize, m_endBound, m_boundCheckSize);
}

void MemorySystem::dumpToFile(bool fullDump) const
{
	std::ofstream l_file;
	l_file.open("../" + g_pTimeSystem->getCurrentTimeInLocalForOutput() + ".innoMemoryDump");

	l_file << "InnoMemory Pool Dump File ----------------------------------\n";
	l_file << "Total Size: " << m_totalPoolSize << "\n";
	l_file << "Free Size: " << m_freePoolSize << "\n";

	// Now search for a block big enough
	Chunk* block = (Chunk*)(m_poolMemory + m_boundCheckSize);

	while (block)
	{
		if (block->m_free)
		{
			l_file << "Free: " << block << "[Bytes: " << block->m_chuckSize << "]\n";
		}
		else
		{
			l_file << "Used: " << block << "[Bytes: " << block->m_chuckSize << "]\n";
		}
		block = block->m_next;
	}

	if (fullDump)
	{
		l_file << "\n\nMemory Dump:\n";
		unsigned char* ptr = m_poolMemory;
		unsigned char* charPtr = m_poolMemory;

		l_file << "Start: " << ptr << "\n";
		unsigned char i = 0;

		// Write the hex memory data
		unsigned long long bytesPerLine = 32;

		l_file << "\n" << ptr << ": " << *(ptr);
		++ptr;
		for (i = 1; ((unsigned long long)(ptr - m_poolMemory) < m_totalPoolSize); ++i, ++ptr)
		{
			if (i == bytesPerLine)
			{
				// Write all the chars for this line now
				l_file << "  " << charPtr;
				for (unsigned long long charI = 0; charI < bytesPerLine; ++charI, ++charPtr)
					l_file << *charPtr;
				charPtr = ptr;

				// Write the new line memory data
				l_file << "\n" << ptr << ": " << *(ptr);
				i = 0;
			}
			else
				l_file << *(ptr);
		}

		// Fill any gaps in the tab
		if ((unsigned long long)(ptr - m_poolMemory) >= m_totalPoolSize)
		{
			unsigned long long lastLineBytes = i;
			for (i; i < bytesPerLine; i++)
				l_file << " --";

			// Write all the chars for this line now
			l_file << "  " << charPtr;
			for (unsigned long long charI = 0; charI < lastLineBytes; ++charI, ++charPtr)
				l_file << *charPtr;
			charPtr = ptr;
		}
	}

	l_file.close();
}

const objectStatus & MemorySystem::getStatus() const
{
	return m_objectStatus;
}
