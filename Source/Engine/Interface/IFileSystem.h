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

	virtual const char* getComponentTypeName(uint32_t typeID) = 0;
	virtual uint32_t getComponentTypeID(const char* typeName) = 0;

	virtual std::string getCurrentSceneName() = 0;
	virtual bool loadScene(const char* fileName, bool AsyncLoad = true) = 0;
	virtual bool saveScene(const char* fileName = "") = 0;
	virtual bool isLoadingScene() = 0;

	virtual bool addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority = -1) = 0;
	virtual bool addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority = -1) = 0;

	virtual bool addCPPClassFiles(const CPPClassDesc& desc) = 0;
};