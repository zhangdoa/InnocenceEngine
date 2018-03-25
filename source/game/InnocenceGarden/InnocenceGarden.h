#pragma once
#include "interface/IGame.h"
#include "entity/BaseEntity.h"
#include "PlayerCharacter.h"

class InnocenceGarden : public IGame
{
public:
	InnocenceGarden();
	~InnocenceGarden();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	std::string getGameName() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	std::vector<CameraComponent*>& getCameraComponents() override;
	std::vector<InputComponent*>& getInputComponents() override;
	std::vector<LightComponent*>& getLightComponents() override;
	std::vector<VisibleComponent*>& getVisibleComponents() override;

	BaseEntity m_rootEntity;

	PlayerCharacter m_playCharacter;
	BaseEntity m_skyboxEntity;
	BaseEntity m_directionalLightEntity;

	BaseEntity m_landscapeEntity;
	BaseEntity m_pawnEntity1;
	BaseEntity m_pawnEntity2;

	std::vector<BaseEntity> m_sphereEntitys;

	std::vector<BaseEntity> m_pointLightEntitys;

	VisibleComponent m_skyboxComponent;

	LightComponent m_directionalLightComponent;
	VisibleComponent m_directionalLightBillboardComponent;

	VisibleComponent m_landscapeComponent;
	VisibleComponent m_pawnComponent1;
	VisibleComponent m_pawnComponent2;

	std::vector<VisibleComponent> m_sphereComponents;

	std::vector<LightComponent> m_pointLightComponents;
	std::vector<VisibleComponent> m_pointLightBillboardComponents;

	std::vector<CameraComponent*> m_cameraComponents;
	std::vector<InputComponent*> m_inputComponents;
	std::vector<LightComponent*> m_lightComponents;
	std::vector<VisibleComponent*> m_visibleComponents;

	double temp = 0.0f;

	void setupSpheres();
	void setupLights();
	void updateLights(double seed);	
	void updateSpheres(double seed);
};

InnocenceGarden g_game;
IGame* g_pGame = &g_game;

