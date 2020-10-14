#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"
#include "../Common/ComponentHeaders.h"

struct CPPClassDesc
{
	bool isInterface = false;
	bool isNonCopyable = true;
	bool isNonMoveable = true;
	std::string parentClass;
	std::string className;
	std::string filePath;
};

class IFileSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IFileSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual std::string getWorkingDirectory() = 0;

	virtual std::vector<char> loadFile(const char* filePath, IOMode openMode) = 0;
	virtual bool saveFile(const char* filePath, const std::vector<char>& content, IOMode saveMode) = 0;

	virtual bool addCPPClassFiles(const CPPClassDesc& desc) = 0;
};