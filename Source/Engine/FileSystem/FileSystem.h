#pragma once
#include "IFileSystem.h"

INNO_CONCRETE InnoFileSystem : INNO_IMPLEMENT IFileSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoFileSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	std::string getWorkingDirectory() override;

	std::vector<char> loadFile(const std::string& filePath, IOMode openMode) override;
	bool saveFile(const std::string& filePath, const std::vector<char>& content, IOMode saveMode) override;

	std::string getCurrentSceneName() override;
	bool loadScene(const std::string& fileName) override;
	bool saveScene(const std::string& fileName) override;
	bool isLoadingScene() override;

	bool addSceneLoadingStartCallback(std::function<void()>* functor) override;
	bool addSceneLoadingFinishCallback(std::function<void()>* functor) override;

	bool convertModel(const std::string & fileName, const std::string & exportPath) override;

	ModelMap loadModel(const std::string & fileName) override;
	TextureDataComponent* loadTexture(const std::string & fileName) override;

	bool addCPPClassFiles(const CPPClassDesc& desc) override;
};
