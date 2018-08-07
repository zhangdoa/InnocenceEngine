#include "MemorySystem.h"

void MemorySystem::setup()
{
	this->setup(m_maxPoolSize);
}

void MemorySystem::setup(unsigned long  memoryPoolSize)
{
	// Allocate memory pool
	m_poolMemoryPtr = nullptr;
	m_poolMemoryPtr = ::new unsigned char[memoryPoolSize];
	std::memset(m_poolMemoryPtr, 0xCC, memoryPoolSize);
	m_availablePoolSize = memoryPoolSize - sizeof(Chunk) - m_boundCheckSize * 2;
	m_totalPoolSize = memoryPoolSize;
	m_availablePoolSize -= m_boundCheckSize * 2;

	// first free chuck
	Chunk freeChunk(memoryPoolSize);
	std::memcpy(m_poolMemoryPtr, m_startBoundMarker, m_boundCheckSize);
	std::memcpy(m_poolMemoryPtr + m_boundCheckSize, &freeChunk, sizeof(Chunk));
	std::memcpy(m_poolMemoryPtr + memoryPoolSize - m_boundCheckSize, m_endBoundMarker, m_boundCheckSize);
}

void MemorySystem::initialize()
{
	g_pLogSystem->printLog(sizeof(Chunk));
	m_objectStatus = objectStatus::ALIVE;
	g_pLogSystem->printLog("MemorySystem has been initialized.");
}

void MemorySystem::update()
{
}

void MemorySystem::shutdown()
{
	::delete[] m_poolMemoryPtr;
}

void * MemorySystem::allocate(unsigned long size)
{
	// add bound check size
	// [StartBound + Chuck + data + EndBound]
	unsigned long l_requiredSize = size + sizeof(Chunk) + m_boundCheckSize * 2;
	// alignment to 4
	// [StartBound + Chuck + data + EndBound + alignment]
	for (size_t i = 0; i < s_NumBlockSizes; i++)
	{
		if (l_requiredSize <= s_BlockSizes[i])
		{
			l_requiredSize = s_BlockSizes[i];
			break;
		}
	}
	// Now search for a block big enough from the beginning of the pool, double linked list, O(n)
	//[                 suitable block                 ] is larger than
	//[StartBoundMarker + Chuck + data + EndBoundMarker + alignment]
	Chunk* l_suitableChuckPtr = reinterpret_cast<Chunk*>(m_poolMemoryPtr + m_boundCheckSize);
	while (l_suitableChuckPtr)
	{
		if (l_suitableChuckPtr->m_free && l_suitableChuckPtr->m_blockSize - l_requiredSize >= m_minFreeBlockSize) { break; }
		l_suitableChuckPtr = l_suitableChuckPtr->m_next;
	}

	// If no block is found, return nullptr
	if (!l_suitableChuckPtr) { return nullptr; }

	// If the block is valid, create a new free block with
	// what remains of the block memory
	unsigned long l_freeBlockSize = l_suitableChuckPtr->m_blockSize - l_requiredSize;
	//[      suitable block       ] is divided to
	//[required block + free block]
	Chunk l_freeChuck(l_freeBlockSize);
	Chunk* l_freeChuckPtr = l_suitableChuckPtr + l_requiredSize;
	// assign the next block's prev pointer to this free block
	if (l_suitableChuckPtr->m_next)
	{
		l_freeChuck.m_next = l_suitableChuckPtr->m_next;
		l_freeChuck.m_next->m_prev = l_freeChuckPtr;
	}
	l_freeChuck.m_prev = l_suitableChuckPtr;

	unsigned char* l_freeBlockPtr = reinterpret_cast<unsigned char*>(l_freeChuckPtr - m_boundCheckSize);
	std::memcpy(l_freeBlockPtr, m_startBoundMarker, m_boundCheckSize);
	std::memcpy(l_freeChuckPtr, &l_freeChuck, sizeof(Chunk));
	std::memcpy(l_freeBlockPtr + l_freeBlockSize - m_boundCheckSize, m_endBoundMarker, m_boundCheckSize);

	// set allocated block chuck data
	l_suitableChuckPtr->m_next = l_freeChuckPtr;
	l_suitableChuckPtr->m_blockSize = l_requiredSize;
	l_suitableChuckPtr->m_free = false;

	// Fill the block with end bound marker and filling flags
	unsigned char* l_suitableBlockPtr = reinterpret_cast<unsigned char*>(l_suitableChuckPtr - m_boundCheckSize);
	std::memset(l_suitableChuckPtr + sizeof(Chunk), 0xAB, size);
	std::memcpy(l_suitableBlockPtr + l_requiredSize - m_boundCheckSize, m_endBoundMarker, m_boundCheckSize);

	// update the pool size
	m_availablePoolSize -= l_requiredSize;

	return (l_suitableChuckPtr + sizeof(Chunk));
}

void MemorySystem::free(void * ptr)
{
	// is a valid node?
	if (!ptr) return;
	Chunk* l_chuckPtr = reinterpret_cast<Chunk*>((unsigned char*)ptr - sizeof(Chunk));
	if (l_chuckPtr->m_free) return;

	unsigned long l_fullFreeBlockSize = l_chuckPtr->m_blockSize;
	m_availablePoolSize += l_fullFreeBlockSize;

	Chunk* l_freeChuckPtr = l_chuckPtr;
	Chunk* l_prevChuckPtr = l_chuckPtr->m_prev;
	Chunk* l_nextChuckPtr = l_chuckPtr->m_next;

	if (l_prevChuckPtr)
	{
		// If the node before is free merge it with this one
		if (l_prevChuckPtr->m_free)
		{
			l_freeChuckPtr = l_prevChuckPtr;
			if (l_freeChuckPtr->m_prev)
			{
				l_prevChuckPtr = l_freeChuckPtr->m_prev;
			}
			else
			{
				l_prevChuckPtr = nullptr;
			}

			// Include the prev node in the block size so we trash it as well
			l_fullFreeBlockSize += l_prevChuckPtr->m_blockSize;
		}
	}
	// If there is a next one, we need to update its pointer
	if (l_nextChuckPtr)
	{
		// We will re point the next of the freer's prev
		l_nextChuckPtr->m_prev = l_freeChuckPtr;

		// Include the next node in the block size if it is
		// free so we trash it as well
		if (l_nextChuckPtr->m_free)
		{
			// We will point to next's next
			if (l_nextChuckPtr->m_next)
			{
				l_fullFreeBlockSize += l_nextChuckPtr->m_blockSize;
				l_nextChuckPtr = l_nextChuckPtr->m_next;
				l_nextChuckPtr->m_prev = l_freeChuckPtr;
			}
			else
			{
				l_nextChuckPtr = nullptr;
			}
		}
	}
	// Create the free block
	unsigned char* l_freeBlockPtr = reinterpret_cast<unsigned char*>(l_freeChuckPtr - sizeof(Chunk));

	Chunk l_freeChuck(l_fullFreeBlockSize);
	l_freeChuck.m_prev = l_prevChuckPtr;
	l_freeChuck.m_next = l_nextChuckPtr;
	std::memcpy(l_freeBlockPtr, m_startBoundMarker, m_boundCheckSize);
	std::memcpy(l_freeBlockPtr + m_boundCheckSize, &l_freeChuck, sizeof(Chunk));
	std::memset(l_freeBlockPtr + m_boundCheckSize + sizeof(Chunk), 0xCC, l_fullFreeBlockSize);
	std::memcpy(l_freeBlockPtr + l_fullFreeBlockSize - m_boundCheckSize, m_endBoundMarker, m_boundCheckSize);
}

void MemorySystem::serializeImpl(void * ptr)
{
	// is a valid node?
	if (!ptr) return;
	Chunk* block = reinterpret_cast<Chunk*>((unsigned char*)ptr - sizeof(Chunk));
	if (block->m_free) return;

	unsigned long l_fullBlockSize = block->m_blockSize + sizeof(Chunk) + m_boundCheckSize * 2;

	std::ofstream l_file;
	l_file.open("../" + g_pTimeSystem->getCurrentTimeInLocalForOutput() + ".innoAssetTest");

	unsigned char* origPtr = (unsigned char*)ptr;
	unsigned char* charPtr = (unsigned char*)ptr;
	unsigned char i = 0;

	// Write the hex memory data
	unsigned long long bytesPerLine = 16;

	l_file << "\n" << origPtr << ": " << *(origPtr);
	++origPtr;
	for (i = 1; ((unsigned long long)(origPtr - m_poolMemoryPtr) < l_fullBlockSize); ++i, ++origPtr)
	{
		if (i == bytesPerLine)
		{
			// Write all the chars for this line now
			l_file << "  " << charPtr;
			for (unsigned long long charI = 0; charI < bytesPerLine; ++charI, ++charPtr)
				l_file << *charPtr;
			charPtr = origPtr;

			// Write the new line memory data
			l_file << "\n" << origPtr << ": " << *(origPtr);
			i = 0;
		}
		else
			l_file << *(origPtr);
	}
	l_file.close();
}

void MemorySystem::dumpToFile(bool fullDump) const
{
	std::ofstream l_file;
	l_file.open("../" + g_pTimeSystem->getCurrentTimeInLocalForOutput() + ".innoMemoryDump");

	l_file << "InnoMemory Pool Dump File ----------------------------------\n";
	l_file << "Total Size: " << m_totalPoolSize << "\n";
	l_file << "Free Size: " << m_availablePoolSize << "\n";

	Chunk* block = (Chunk*)(m_poolMemoryPtr + m_boundCheckSize);

	while (block)
	{
		if (block->m_free)
		{
			l_file << "Free: " << block << "[Bytes: " << block->m_blockSize << "]\n";
		}
		else
		{
			l_file << "Used: " << block << "[Bytes: " << block->m_blockSize << "]\n";
		}
		block = block->m_next;
	}

	if (fullDump)
	{
		l_file << "\n\nMemory Dump:\n";
		unsigned char* ptr = m_poolMemoryPtr;
		unsigned char* charPtr = m_poolMemoryPtr;

		l_file << "Start: " << ptr << "\n";
		unsigned char i = 0;

		// Write the hex memory data
		unsigned long long bytesPerLine = 32;

		l_file << "\n" << ptr << ": " << *(ptr);
		++ptr;
		for (i = 1; ((unsigned long long)(ptr - m_poolMemoryPtr) < m_totalPoolSize); ++i, ++ptr)
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
		if ((unsigned long long)(ptr - m_poolMemoryPtr) >= m_totalPoolSize)
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
