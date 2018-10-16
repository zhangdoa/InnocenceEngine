#pragma once
#include "BaseComponent.h"
#include "../common/InnoConcurrency.h"

class GameSystemSingletonComponent : public BaseComponent
{
public:
	~GameSystemSingletonComponent() {};
	
	static GameSystemSingletonComponent& getInstance()
	{
		static GameSystemSingletonComponent instance;
		return instance;
	}

	// the SOA here
	std::vector<TransformComponent*> m_transformComponents;
	std::vector<VisibleComponent*> m_visibleComponents;
	std::vector<LightComponent*> m_lightComponents;
	std::vector<CameraComponent*> m_cameraComponents;
	std::vector<InputComponent*> m_inputComponents;
	std::vector<EnvironmentCaptureComponent*> m_environmentCaptureComponents;

	std::unordered_map<EntityID, TransformComponent*> m_TransformComponentsMap;
	std::unordered_multimap<EntityID, VisibleComponent*> m_VisibleComponentsMap;
	std::unordered_multimap<EntityID, LightComponent*> m_LightComponentsMap;
	std::unordered_multimap<EntityID, CameraComponent*> m_CameraComponentsMap;
	std::unordered_multimap<EntityID, InputComponent*> m_InputComponentsMap;
	std::unordered_multimap<EntityID, EnvironmentCaptureComponent*> m_EnvironmentCaptureComponentsMap;

	bool m_needRender = true;

	InnoFuture<void>* m_asyncTask;

private:
	GameSystemSingletonComponent() {};
};
