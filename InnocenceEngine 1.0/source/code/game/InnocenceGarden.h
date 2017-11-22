#pragma once
#include "../core/interface/IGameData.h"
#include "../core/interface/IGameEntity.h"
#include "../core/manager/CoreManager.h"

#include "PlayerCharacter.h"
class InnocenceGarden : public IGameData
{
public:
	InnocenceGarden();
	~InnocenceGarden();

private:
	void initialize() override;
	void update() override;
	void shutdown() override;

	void initSpheres();
	void loadTextureForSpheres();
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

	BaseActor sp1;
	BaseActor sp2;
	BaseActor sp3;
	BaseActor sp4;
	BaseActor sp5;
	BaseActor sp6;
	BaseActor sp7;
	BaseActor sp8;
	std::vector<BaseActor> sphereActors = { sp1, sp2, sp3, sp4, sp5, sp6, sp7, sp8 };

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

	VisibleComponent sp1Component;
	VisibleComponent sp2Component;
	VisibleComponent sp3Component;
	VisibleComponent sp4Component;
	VisibleComponent sp5Component;
	VisibleComponent sp6Component;
	VisibleComponent sp7Component;
	VisibleComponent sp8Component;
	std::vector<VisibleComponent> sphereComponents = { sp1Component , sp2Component , sp3Component , sp4Component , sp5Component , sp6Component , sp7Component , sp8Component };
	double temp = 0.0f;
};

