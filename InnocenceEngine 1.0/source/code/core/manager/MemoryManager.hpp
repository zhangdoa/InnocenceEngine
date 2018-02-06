#pragma once
#include "../interface/IManager.h"
#include "LogManager.h"

class MemoryManager : public IManager
{
public:
	~MemoryManager() {};

	void setup() override;
	void setup(unsigned long  memoryPoolSize);
	void initialize() override;
	void update() override;
	void shutdown() override;

	inline void* allocate(unsigned long size);
	
	inline void free(void* ptr);

	template <typename T>
	T * spawn(void)
	{
		return reinterpret_cast<T *>(allocate(sizeof(T)));
	}

	template <typename T>
	void destroy(T *p)
	{
		reinterpret_cast<T *>(p)->~T();
		free(p);
	}

	inline void dumpToFile(const std::string& fileName) const;

	static MemoryManager& getInstance()
	{
		static MemoryManager instance;
		return instance;
	}

private:
	MemoryManager() {};

	//const unsigned long  m_maxPoolSize = 1024 * 1024 * 1024;
	const unsigned long  m_maxPoolSize = 1024 * 1024;
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

