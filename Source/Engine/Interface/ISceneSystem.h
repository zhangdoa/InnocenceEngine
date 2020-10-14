#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Common/InnoObject.h"

using SceneHierarchyMap = std::unordered_map<InnoEntity*, std::set<InnoComponent*>>;

class ISceneSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ISceneSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual std::string getCurrentSceneName() = 0;
	virtual bool loadScene(const char* fileName, bool AsyncLoad = true) = 0;
	virtual bool saveScene(const char* fileName = "") = 0;
	virtual bool isLoadingScene() = 0;

	virtual bool addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority = -1) = 0;
	virtual bool addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority = -1) = 0;

	virtual const SceneHierarchyMap& getSceneHierarchyMap() = 0;
};