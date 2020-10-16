#pragma once
#include "ISystem.h"

using SceneHierarchyMap = std::unordered_map<InnoEntity*, std::set<InnoComponent*>>;

class ISceneSystem : public ISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ISceneSystem);

	virtual std::string getCurrentSceneName() = 0;
	virtual bool loadScene(const char* fileName, bool AsyncLoad = true) = 0;
	virtual bool saveScene(const char* fileName = "") = 0;
	virtual bool isLoadingScene() = 0;

	virtual bool addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority = -1) = 0;
	virtual bool addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority = -1) = 0;

	virtual const SceneHierarchyMap& getSceneHierarchyMap() = 0;
};