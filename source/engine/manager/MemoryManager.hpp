#pragma once
#include "BaseManager.h"
#include "LogManager.h"

	static const uint32_t s_BlockSizes[] = {
		// 4-increments
		4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48,
		52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96,

		// 32-increments
		128, 160, 192, 224, 256, 288, 320, 352, 384,
		416, 448, 480, 512, 544, 576, 608, 640,

		// 64-increments
		704, 768, 832, 896, 960, 1024
	};
	// number of elements in the block size array
	static const uint32_t s_NumBlockSizes =
		sizeof(s_BlockSizes) / sizeof(s_BlockSizes[0]);

	// largest valid block size
	static const uint32_t s_MaxBlockSize =
		s_BlockSizes[s_NumBlockSizes - 1];

class MemoryManager : public BaseManager
{
public:
	~MemoryManager() {};

	void setup() override;
	void setup(unsigned long  memoryPoolSize);
	void initialize() override;
	void update() override;
	void shutdown() override;

	template <typename T>
	T * spawn(void)
	{
		for (size_t i = 0; i < s_MaxBlockSize; i++)
		{

		}
		sizeof(T);
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

	inline void* allocate(unsigned long size);

	inline void free(void* ptr);

	const unsigned long  m_maxPoolSize = 1024;
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

