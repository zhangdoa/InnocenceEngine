#pragma once
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
#include "interface/ITimeSystem.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>

extern ILogSystem* g_pLogSystem;
extern ITimeSystem* g_pTimeSystem;

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

class MemorySystem : public IMemorySystem
{
public:
	MemorySystem() {};
	~MemorySystem() {};

	void setup() override;
	void setup(unsigned long  memoryPoolSize);
	void initialize() override;
	void update() override;
	void shutdown() override;

	void* allocate(unsigned long size) override;
	void free(void* ptr) override;
	void serializeImpl(void* ptr) override;
	void* deserializeImpl(unsigned long size, const std::string& filePath) override;

	void dumpToFile(bool fullDump) const override;
	
	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	const unsigned long  m_maxPoolSize = 1024 * 1024 * 512;
	static const unsigned char m_minFreeBlockSize = 48;
	unsigned long  m_totalPoolSize;
	unsigned long  m_availablePoolSize;

	static const unsigned int m_boundCheckSize = 16;
	const unsigned char  m_startBoundMarker[m_boundCheckSize] = { '[','I','n','n','o','C','h','u','c','k','S','t','a','r','t',']' };
	const unsigned char  m_endBoundMarker[m_boundCheckSize] = { '[','I','n','n','o','C','h','u','c','k','.','.','E','n','d',']' };

	unsigned char* m_poolMemoryPtr = nullptr;

	class Chunk
	{
	public:
		Chunk(unsigned int chuckSize) : m_next(nullptr),
			m_prev(nullptr),
			m_blockSize(chuckSize),
			m_free(true) {};

		Chunk*  m_next;
		Chunk*  m_prev;
		unsigned int   m_blockSize;
		bool    m_free;
	};
};