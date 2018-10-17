#pragma once
#include "../../common/InnoType.h"

namespace InnoMemorySystem
{
	__declspec(dllexport) void setup();
	__declspec(dllexport) void initialize();
	__declspec(dllexport) void update();
	__declspec(dllexport) void shutdown();

	__declspec(dllexport) void* allocate(unsigned long size);
	__declspec(dllexport) void free(void* ptr);
	__declspec(dllexport) void serializeImpl(void* ptr);
	__declspec(dllexport) void* deserializeImpl(unsigned long size, const std::string& filePath);

	__declspec(dllexport) void dumpToFile(bool fullDump);
	
    template <typename T> __declspec(dllexport) T * spawn()
    {
        return new(allocate(sizeof(T))) T();
    };
    
    template <typename T> __declspec(dllexport) T * spawn(size_t n)
    {
        return reinterpret_cast<T *>(allocate(n * sizeof(T)));
    };
    
    template <typename T> __declspec(dllexport) void destroy(T *p)
    {
        reinterpret_cast<T *>(p)->~T();
        free(p);
    };
    
    template <typename T> __declspec(dllexport) void serialize(T* p)
    {
        serializeImpl(p);
    };
    
    template <typename T> __declspec(dllexport) T* deserialize(const std::string& filePath)
    {
        return reinterpret_cast<T *>(deserializeImpl(sizeof(T), filePath));
    };
   
	__declspec(dllexport) objectStatus getStatus();
};
