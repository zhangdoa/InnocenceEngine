#pragma once
#include "../Interface/ISceneSystem.h"

namespace Inno
{
	class SceneSystem : public ISceneSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(SceneSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		std::string getCurrentSceneName() override;
		bool loadScene(const char* fileName, bool AsyncLoad) override;
		bool saveScene(const char* fileName) override;
		bool isLoadingScene() override;

		bool addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority) override;
		bool addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority) override;

		const SceneHierarchyMap& getSceneHierarchyMap() override;
	};
}