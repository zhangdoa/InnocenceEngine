#pragma once
#include "../Interface/ISystem.h"

namespace Inno
{
	using SceneLoadingCallback = std::pair<std::function<void()>*, int32_t>;
	using ComponentPair = std::pair<uint32_t, Component*>;
	using SceneHierarchyMap = std::unordered_map<Entity*, std::set<ComponentPair>>;
	class SceneService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(SceneService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus();

		std::string GetCurrentSceneName();
		bool Load(const char* fileName, bool AsyncLoad = false);
		bool Save(const char* fileName);
		bool IsLoading();

		bool AddSceneLoadingStartedCallback(std::function<void()>* functor, int32_t priority);
		bool AddSceneLoadingFinishedCallback(std::function<void()>* functor, int32_t priority);

		const SceneHierarchyMap& getSceneHierarchyMap();

	private:
		bool LoadAsync(const char* fileName);
		bool LoadSync(const char* fileName);
		
		template<typename T>
		void AddComponentToSceneHierarchyMap();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		std::vector<SceneLoadingCallback> m_sceneLoadingStartCallbacks;
		std::vector<SceneLoadingCallback> m_sceneLoadingFinishCallbacks;

		std::atomic<bool> m_IsLoading = false;
		std::atomic<bool> m_prepareForLoadingScene = false;

		std::string m_nextLoadingScene;
		std::string m_currentScene;

		SceneHierarchyMap m_SceneHierarchyMap;
		std::atomic<bool> m_needUpdate = true;

		std::function<void()> f_SceneLoadingStartedCallback;
		std::function<void()> f_SceneLoadingFinishCallback;
	};
}