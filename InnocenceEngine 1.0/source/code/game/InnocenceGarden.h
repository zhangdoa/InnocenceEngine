#pragma once
#include "../core/interface/IGameData.h"
#include "../core/interface/IGameEntity.h"

#include "../core/component/LightComponent.h"
#include "../core/component/VisibleComponent.h"

#include "PlayerCharacter.h"
class InnocenceGarden :	public IGameData
{
public:
	InnocenceGarden();
	~InnocenceGarden();

private:
	void initialize() override;
	void update() override;
	void shutdown() override;

	BaseActor rootActor;

	PlayerCharacter playCharacter;
	BaseActor skyboxActor;
	BaseActor directionalLightActor;
	BaseActor pointLightActor1;
	BaseActor pointLightActor2;
	BaseActor pointLightActor3;

	BaseActor landscapeStaticMeshActor;
	BaseActor testStaticMeshActor1;
	BaseActor testStaticMeshActor2;

	VisibleComponent testSkyboxComponent;

	LightComponent directionalLightComponent;
	LightComponent pointLightComponent1;
	LightComponent pointLightComponent2;
	LightComponent pointLightComponent3;

	VisibleComponent pointLightBillboardComponent1;
	VisibleComponent pointLightBillboardComponent2;
	VisibleComponent pointLightBillboardComponent3;

	VisibleComponent landscapeStaticMeshComponent;
	VisibleComponent testStaticMeshComponent1;
	VisibleComponent testStaticMeshComponent2;

	double temp = 0.0f;
};

