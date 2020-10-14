#pragma once
#include "../Interface/IFileSystem.h"

class InnoFileSystem : public IFileSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoFileSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	std::string getWorkingDirectory() override;

	std::vector<char> loadFile(const char* filePath, IOMode openMode) override;
	bool saveFile(const char* filePath, const std::vector<char>& content, IOMode saveMode) override;

	bool addCPPClassFiles(const CPPClassDesc& desc) override;
};
