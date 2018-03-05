#pragma once
#include "common/stdafx.h"
#include "IManager.h"

class IMemoryManager : public IManager
{
public:
	virtual ~IMemoryManager() {};

	virtual void* allocate(unsigned long size) = 0;
	virtual void free(void* ptr) = 0;
	template <typename T> virtual T * spawn() = 0;
	template <typename T> virtual void destroy(T *p) = 0;
	virtual void dumpToFile(const std::string& fileName) const = 0;
};

IMemoryManager* g_pMemoryManager;