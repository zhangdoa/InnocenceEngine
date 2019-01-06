#pragma once
#include "IFileSystem.h"

class InnoFileSystem : INNO_IMPLEMENT IFileSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoFileSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT std::string loadTextFile(const std::string& fileName) override;

protected:
	INNO_SYSTEM_EXPORT void saveComponentToDiskImpl(componentType type, size_t classSize, void* ptr, const std::string& fileName) override;
	INNO_SYSTEM_EXPORT void* loadComponentFromDiskImpl(const std::string& fileName) override;
};
