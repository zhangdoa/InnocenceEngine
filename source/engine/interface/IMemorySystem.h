#pragma once
#include "common/stdafx.h"
#include "ISystem.h"

class IMemorySystem : public ISystem
{
public:
	virtual ~IMemorySystem() {};

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

	virtual void dumpToFile(bool fullDump) const = 0;

protected:
	virtual void* allocate(unsigned long size) = 0;
	virtual void free(void* ptr) = 0;
	virtual void serializeImpl(void* ptr) = 0;
	virtual void* deserializeImpl(unsigned long size, const std::string& filePath) = 0;
};