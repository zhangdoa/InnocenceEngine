#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../common/ComponentHeaders.h"

INNO_INTERFACE IFileSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IFileSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

protected:
	INNO_SYSTEM_EXPORT virtual void saveComponentToDiskImpl(componentType type, size_t classSize, void* ptr, const std::string& fileName) = 0;
	INNO_SYSTEM_EXPORT virtual void* loadComponentFromDiskImpl(const std::string& fileName) = 0;

public:
	template <typename T> 
	void saveComponentToDisk(T* p, const std::string& fileName)
	{
		saveComponentToDiskImpl(InnoUtility::getComponentType<T>(), sizeof(T), p, fileName);
	};

	template <typename T> 
	T* loadComponentFromDisk(const std::string& fileName)
	{
		return reinterpret_cast<T *>(loadComponentFromDiskImpl(fileName));
	};

	INNO_SYSTEM_EXPORT virtual std::string loadTextFile(const std::string& fileName) = 0;

	INNO_SYSTEM_EXPORT virtual bool loadDefaultScene() = 0;
	INNO_SYSTEM_EXPORT virtual bool loadScene(const std::string& fileName) = 0;
	INNO_SYSTEM_EXPORT virtual bool saveScene(const std::string& fileName) = 0;
};