#pragma once
#include "system/GameSystem.h"
#include "PlayerCharacter.h"
#include "component/InnoAllocator.h"

class InnocenceGarden : public GameSystem
{
public:
	InnocenceGarden() {};
	~InnocenceGarden() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	std::string getGameName() const override;
	std::vector<TransformComponent*>& getTransformComponents() override;
	std::vector<CameraComponent*>& getCameraComponents() override;
	std::vector<InputComponent*>& getInputComponents() override;
	std::vector<LightComponent*>& getLightComponents() override;
	std::vector<VisibleComponent*>& getVisibleComponents() override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	// root entity and its components
	EntityID m_rootEntity;
	TransformComponent* m_rootTransformComponent;

	// player character
	EntityID m_playerCharacterEntity;
	PlayerCharacter* m_playerCharacterComponent;

	// skybox entity and its components
	EntityID m_skyboxEntity;
	TransformComponent* m_skyboxTransformComponent;
	VisibleComponent* m_skyboxVisibleComponent;

	// directional light/ sun entity and its components
	EntityID m_directionalLightEntity;
	TransformComponent* m_directionalLightTransformComponent;
	LightComponent* m_directionalLightComponent;
	VisibleComponent* m_directionalLightVisibleComponent;

	// landscape entity and its components
	EntityID m_landscapeEntity;
	TransformComponent* m_landscapeTransformComponent;
	VisibleComponent* m_landscapeVisibleComponent;
	
	// pawn entity 1 and its components
	EntityID m_pawnEntity1;
	TransformComponent* m_pawnTransformComponent1;
	VisibleComponent* m_pawnVisibleComponent1;

	// pawn entity 2 and its components
	EntityID m_pawnEntity2;
	TransformComponent* m_pawnTransformComponent2;
	VisibleComponent* m_pawnVisibleComponent2;

	// sphere entities and their components
	std::vector<EntityID> m_sphereEntitys;
	std::vector<TransformComponent*> m_sphereTransformComponents;
	std::vector<VisibleComponent*> m_sphereVisibleComponents;

	// punctual point light entities and their components
	std::vector<EntityID> m_pointLightEntitys;
	std::vector<TransformComponent*> m_pointLightTransformComponents;
	std::vector<LightComponent*> m_pointLightComponents;
	std::vector<VisibleComponent*> m_pointLightVisibleComponents;

	double temp = 0.0f;

	void setupSpheres();
	void setupLights();
	void updateLights(double seed);	
	void updateSpheres(double seed);
};
