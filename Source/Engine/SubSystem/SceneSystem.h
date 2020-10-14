#pragma once
#include "../Interface/ISceneSystem.h"

class InnoSceneSystem : public ISceneSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoSceneSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	std::string getCurrentSceneName() override;
	bool loadScene(const char* fileName, bool AsyncLoad) override;
	bool saveScene(const char* fileName) override;
	bool isLoadingScene() override;

	bool addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority) override;
	bool addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority) override;

	const SceneHierarchyMap& getSceneHierarchyMap() override;
};