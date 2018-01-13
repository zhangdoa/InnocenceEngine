#pragma once
#include "../interface/IEventManager.h"
#include "LogManager.h"

class MemoryManager : public IEventManager
{
public:
	~MemoryManager() {};

	void setup() override;
	void setup(unsigned int memoryPoolSize);
	void initialize() override;
	void update() override;
	void shutdown() override;

	inline void* allocate(unsigned int size);
	inline void free(void* ptr);

	static MemoryManager& getInstance()
	{
		static MemoryManager instance;
		return instance;
	}

private:
	MemoryManager() {};

	const int m_maxMemoryPoolSize = 1024 * 1024 * 1024;
	const unsigned char  MemoryManager::m_startBound[16] = { '[','I','n','n','o','C','h','u','c','k','S','t','a','r','t',']' };
	const unsigned char  MemoryManager::m_endBound[16] = { '[','I','n','n','o','C','h','u','c','k','.','.','E','n','d',']' };

	unsigned char* m_poolMemory;

	class Chunk
	{
	public:
		Chunk(unsigned int chuckSize) : m_next(NULL),
			m_prev(NULL),
			m_chuckSize(chuckSize),
			m_free(true) {};
		void write(void* dest)
		{
			memcpy(dest, this, sizeof(Chunk));
		}
		void read(void* src)
		{
			memcpy(this, src, sizeof(Chunk));
		}

		Chunk*  m_next;
		Chunk*  m_prev;
		unsigned int   m_chuckSize;
		bool    m_free;
	};
};

