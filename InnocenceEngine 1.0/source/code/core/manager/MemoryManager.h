#pragma once
#include "../interface/IEventManager.h"
#include "LogManager.h"

class MemoryManager : public IEventManager
{
public:
	~MemoryManager() {};

	void setup() override;
	void setup(unsigned long  memoryPoolSize);
	void initialize() override;
	void update() override;
	void shutdown() override;


	inline void* allocate(unsigned long  size);
	inline void free(void* ptr);
	inline void dumpToFile(const std::string& fileName) const;

	static MemoryManager& getInstance()
	{
		static MemoryManager instance;
		return instance;
	}

private:
	MemoryManager() {};

	//const unsigned long  m_maxPoolSize = 1024 * 1024 * 1024;
	const unsigned long  m_maxPoolSize = 256;
	static const unsigned char m_minFreeBlockSize = 16;
	unsigned long  m_totalPoolSize;
	unsigned long  m_freePoolSize;

	static const unsigned int m_boundCheckSize = 16;
	const unsigned char  m_startBound[m_boundCheckSize] = { '[','I','n','n','o','C','h','u','c','k','S','t','a','r','t',']' };
	const unsigned char  m_endBound[m_boundCheckSize] = { '[','I','n','n','o','C','h','u','c','k','.','.','E','n','d',']' };

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

