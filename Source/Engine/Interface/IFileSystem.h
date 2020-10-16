#pragma once
#include "ISystem.h"
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

class IFileSystem : public ISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IFileSystem);

	virtual std::string getWorkingDirectory() = 0;

	virtual std::vector<char> loadFile(const char* filePath, IOMode openMode) = 0;
	virtual bool saveFile(const char* filePath, const std::vector<char>& content, IOMode saveMode) = 0;

	virtual bool addCPPClassFiles(const CPPClassDesc& desc) = 0;
};