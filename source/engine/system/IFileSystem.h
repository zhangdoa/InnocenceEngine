#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../common/ComponentHeaders.h"

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

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual std::string getWorkingDirectory() = 0;

	INNO_SYSTEM_EXPORT virtual std::string loadTextFile(const std::string& fileName) = 0;
	INNO_SYSTEM_EXPORT virtual std::vector<char> loadBinaryFile(const std::string& fileName) = 0;

	INNO_SYSTEM_EXPORT virtual bool loadScene(const std::string& fileName) = 0;
	INNO_SYSTEM_EXPORT virtual bool saveScene(const std::string& fileName) = 0;
	INNO_SYSTEM_EXPORT virtual bool isLoadingScene() = 0;

	INNO_SYSTEM_EXPORT virtual bool addSceneLoadingStartCallback(std::function<void()>* functor) = 0;
	INNO_SYSTEM_EXPORT virtual bool addSceneLoadingFinishCallback(std::function<void()>* functor) = 0;

	INNO_SYSTEM_EXPORT virtual bool convertModel(const std::string & fileName, const std::string & exportPath) = 0;

	INNO_SYSTEM_EXPORT virtual ModelMap loadModel(const std::string & fileName) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* loadTexture(const std::string & fileName) = 0;

	INNO_SYSTEM_EXPORT virtual bool addCPPClassFiles(const CPPClassDesc& desc) = 0;
};