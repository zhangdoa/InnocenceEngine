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

	std::vector<TransformComponent*> m_TransformComponents;
	std::vector<VisibleComponent*> m_VisibleComponents;
	std::vector<LightComponent*> m_LightComponents;
	std::vector<CameraComponent*> m_CameraComponents;
	std::vector<InputComponent*> m_InputComponents;
	std::vector<EnvironmentCaptureComponent*> m_EnvironmentCaptureComponents;

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
