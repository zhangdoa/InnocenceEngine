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

	std::string getCurrentSceneName() override;
	bool loadScene(const char* fileName, bool AsyncLoad) override;
	bool saveScene(const char* fileName) override;
	bool isLoadingScene() override;

	bool addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority) override;
	bool addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority) override;

	bool addCPPClassFiles(const CPPClassDesc& desc) override;
};
