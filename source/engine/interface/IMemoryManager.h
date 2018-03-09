#pragma once
#include "common/stdafx.h"
#include "IManager.h"

class IMemoryManager : public IManager
{
public:
	virtual ~IMemoryManager() {};

	virtual void* allocate(unsigned long size) = 0;
	virtual void free(void* ptr) = 0;
	template <typename T> T * spawn()
	{
		return new(allocate(sizeof(T))) T();
	};
	template <typename T> void destroy(T *p)
	{
		reinterpret_cast<T *>(p)->~T();
		free(p);
	};
	virtual void dumpToFile(const std::string& fileName) const = 0;
};