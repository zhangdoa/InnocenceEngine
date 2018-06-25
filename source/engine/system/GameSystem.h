#pragma once
#include "interface/IGameSystem.h"
#include "interface/ITimeSystem.h"

#include "common/ComponentHeaders.h"

extern ITimeSystem* g_pTimeSystem;

class GameSystem : public IGameSystem
{
public:
	GameSystem() {};
	~GameSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	std::vector<TransformComponent*>& getTransformComponents() override;
	std::vector<VisibleComponent*>& getVisibleComponents() override;
	std::vector<LightComponent*>& getLightComponents() override;
	std::vector<CameraComponent*>& getCameraComponents() override;
	std::vector<InputComponent*>& getInputComponents() override;

	TransformComponent* getTransformComponent(IEntity* parentEntity) override;

	std::string getGameName() const override;
	bool needRender() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::vector<TransformComponent*> m_TransformComponents;
	std::vector<VisibleComponent*> m_VisibleComponents;
	std::vector<LightComponent*> m_LightComponents;
	std::vector<CameraComponent*> m_CameraComponents;
	std::vector<InputComponent*> m_InputComponents;

	std::unordered_map<IEntity*, TransformComponent*> m_TransformComponentsMap;
	std::unordered_multimap<IEntity*, VisibleComponent*> m_VisibleComponentsMap;
	std::unordered_multimap<IEntity*, LightComponent*> m_LightComponentsMap;
	std::unordered_multimap<IEntity*, CameraComponent*> m_CameraComponentsMap;
	std::unordered_multimap<IEntity*, InputComponent*> m_InputComponentsMap;

	bool m_needRender = true;
};
