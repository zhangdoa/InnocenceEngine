#pragma once
#include "../../Common/InnoType.h"

#include "../../Common/InnoClassTemplate.h"
#include "../../Common/ComponentHeaders.h"

struct CPPClassDesc
{
	bool isInterface = false;
	bool isNonCopyable = true;
	bool isNonMoveable = true;
	std::string parentClass;
	std::string className;
	std::string filePath;
};

INNO_INTERFACE IFileSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IFileSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual std::string getWorkingDirectory() = 0;

	virtual std::string loadTextFile(const std::string& fileName) = 0;
	virtual std::vector<char> loadBinaryFile(const std::string& fileName) = 0;

	virtual bool loadScene(const std::string& fileName) = 0;
	virtual bool saveScene(const std::string& fileName) = 0;
	virtual bool isLoadingScene() = 0;

	virtual bool addSceneLoadingStartCallback(std::function<void()>* functor) = 0;
	virtual bool addSceneLoadingFinishCallback(std::function<void()>* functor) = 0;

	virtual bool convertModel(const std::string & fileName, const std::string & exportPath) = 0;

	virtual ModelMap loadModel(const std::string & fileName) = 0;
	virtual TextureDataComponent* loadTexture(const std::string & fileName) = 0;

	virtual bool addCPPClassFiles(const CPPClassDesc& desc) = 0;
};