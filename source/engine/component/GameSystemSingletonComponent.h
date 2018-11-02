#pragma once
#include "../common/InnoType.h"
#include "../common/InnoConcurrency.h"

class GameSystemSingletonComponent
{
public:
	~GameSystemSingletonComponent() {};
	
	static GameSystemSingletonComponent& getInstance()
	{
		static GameSystemSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	// the AOS here
	TransformComponent* m_rootTransformComponent;
	std::vector<std::vector<TransformComponent*>> m_currentTransformComponentsTree;
	std::vector<std::vector<TransformComponent*>> m_previousTransformComponentsTree;

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

	InnoFuture<void>* m_asyncTask;

private:
	GameSystemSingletonComponent() {};
};
