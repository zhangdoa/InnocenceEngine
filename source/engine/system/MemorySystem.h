#pragma once
#include "../common/InnoType.h"

namespace InnoMemorySystem
{
	void setup();
	void setup(unsigned long  memoryPoolSize);
	void initialize();
	void update();
	void shutdown();

	template <typename T> T * spawn()
	{
		return new(allocate(sizeof(T))) T();
	};

	template <typename T> T * spawn(size_t n)
	{
		return reinterpret_cast<T *>(allocate(n * sizeof(T)));
	};

	template <typename T> void destroy(T *p)
	{
		reinterpret_cast<T *>(p)->~T();
		free(p);
	};

	template <typename T> void serialize(T* p)
	{
		serializeImpl(p);
	};

	template <typename T> T* deserialize(const std::string& filePath)
	{
		return reinterpret_cast<T *>(deserializeImpl(sizeof(T), filePath));
	};

	void* allocate(unsigned long size);
	void free(void* ptr);
	void serializeImpl(void* ptr);
	void* deserializeImpl(unsigned long size, const std::string& filePath);

	void dumpToFile(bool fullDump);
	
	objectStatus m_MemorySystemStatus = objectStatus::SHUTDOWN;
};