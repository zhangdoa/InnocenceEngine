#pragma once
#include "common/stdafx.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include "ISystem.h"

class IMemorySystem : public ISystem
{
public:
	virtual ~IMemorySystem() {};

	virtual void* allocate(unsigned long size) = 0;
	virtual void free(void* ptr) = 0;
	virtual void serializeImpl(void* ptr) = 0;
	//std::ofstream l_serializedFile;
	//l_serializedFile.open("testCameraComponent.innoAsset", std::ios::out | std::ios::trunc | std::ios::binary);
	////l_serializedFile.write();
	template <typename T> T * spawn()
	{
		return new(allocate(sizeof(T))) T();
	};
	template <typename T> void destroy(T *p)
	{
		reinterpret_cast<T *>(p)->~T();
		free(p);
	};
	template <typename T> void serialize(T *p)
	{
		serializeImpl(reinterpret_cast<void *>(p));
	};
	virtual void dumpToFile(bool fullDump) const = 0;
};