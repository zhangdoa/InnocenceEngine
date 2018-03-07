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
	const objectStatus& getStatus() const;

	void getGameName(std::string & gameName) const override;

protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	std::vector<CameraComponent*>& getCameraComponents() override;
	std::vector<InputComponent*>& getInputComponents() override;
	std::vector<LightComponent*>& getLightComponents() override;
	std::vector<VisibleComponent*>& getVisibleComponents() override;

	BaseActor rootActor;

	PlayerCharacter playCharacter;
	BaseActor skyboxActor;
	BaseActor directionalLightActor;

	BaseActor landscapeActor;
	BaseActor pawnActor1;
	BaseActor pawnActor2;

	std::vector<BaseActor> sphereActors;

	std::vector<BaseActor> pointLightActors;

	VisibleComponent skyboxComponent;

	LightComponent directionalLightComponent;

	VisibleComponent landscapeStaticMeshComponent;
	VisibleComponent pawnMeshComponent1;
	VisibleComponent pawnMeshComponent2;

	std::vector<VisibleComponent> sphereComponents;

	std::vector<LightComponent> pointLightComponents;

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

