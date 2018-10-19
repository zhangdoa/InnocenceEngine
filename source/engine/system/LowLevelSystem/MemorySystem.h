#pragma once
#include "../../common/InnoType.h"
#include "../../exports/LowLevelSystem_Export.h"

namespace InnoMemorySystem
{
	InnoLowLevelSystem_EXPORT bool setup();
	InnoLowLevelSystem_EXPORT bool initialize();
	InnoLowLevelSystem_EXPORT bool update();
	InnoLowLevelSystem_EXPORT bool terminate();

	InnoLowLevelSystem_EXPORT void* allocate(unsigned long size);
	InnoLowLevelSystem_EXPORT void free(void* ptr);
	InnoLowLevelSystem_EXPORT void serializeImpl(void* ptr);
	InnoLowLevelSystem_EXPORT void* deserializeImpl(unsigned long size, const std::string& filePath);

	InnoLowLevelSystem_EXPORT void dumpToFile(bool fullDump);
	
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
   
	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};
