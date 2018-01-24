#pragma once
#include "../core/interface/IGameData.h"
#include "../core/interface/BaseEntity.h"
#include "../core/manager/CoreManager.h"

#include "PlayerCharacter.h"
class InnocenceGarden : public IGameData
{
public:
	InnocenceGarden();
	~InnocenceGarden();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	BaseActor rootActor;

	PlayerCharacter playCharacter;
	BaseActor skyboxActor;
	BaseActor directionalLightActor;
	BaseActor pointLightActor1;
	BaseActor pointLightActor2;
	BaseActor pointLightActor3;

	BaseActor landscapeActor;
	BaseActor pawnActor1;
	BaseActor pawnActor2;

	std::vector<BaseActor> sphereActors;

	std::vector<BaseActor> pointLightActors;

	VisibleComponent skyboxComponent;

	LightComponent directionalLightComponent;
	LightComponent pointLightComponent1;
	LightComponent pointLightComponent2;
	LightComponent pointLightComponent3;

	VisibleComponent pointLightBillboardComponent1;
	VisibleComponent pointLightBillboardComponent2;
	VisibleComponent pointLightBillboardComponent3;

	VisibleComponent landscapeStaticMeshComponent;
	VisibleComponent pawnMeshComponent1;
	VisibleComponent pawnMeshComponent2;

	std::vector<VisibleComponent> sphereComponents;

	std::vector<LightComponent> pointLightComponents;

	float temp = 0.0f;

	void setupSpheres();
	void initializeSpheres();
	void setupLights();
	void updateLights(float seed);	
	void updateSpheres(float seed);
};

