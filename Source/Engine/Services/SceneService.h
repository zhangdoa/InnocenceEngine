#pragma once
#include "../Common/Array.h"
#include "../Interface/IService.h"
#include "../Common/EntityID.h"

namespace Inno
{
	class SceneService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(SceneService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus();

		std::string GetCurrentSceneName();
		bool Load(const char* fileName, bool AsyncLoad = false);
		bool Save(const char* fileName);
		bool IsLoading();
		void ClearLoadingFlag();

		bool AddSceneUnloadingCallback(std::function<void()>* functor);
		bool AddSceneLoadedCallback(std::function<void()>* functor);

	private:
		bool LoadAsync(const char* fileName);
		bool LoadSync(const char* fileName);

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		Inno::Array<std::function<void()>*> m_sceneUnloadingCallbacks;
		Inno::Array<std::function<void()>*> m_sceneLoadedCallbacks;

		std::atomic<bool> m_IsLoading = false;
		std::atomic<bool> m_prepareForLoadingScene = false;

		std::string m_nextLoadingScene;
		std::string m_currentScene;

		std::atomic<bool> m_needUpdate = true;
	};
}
