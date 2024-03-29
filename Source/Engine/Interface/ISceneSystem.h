#pragma once
#include "ISystem.h"

namespace Inno
{
	using ComponentPair = std::pair<uint32_t, Component*>;
	using SceneHierarchyMap = std::unordered_map<Entity*, std::set<ComponentPair>>;

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
}