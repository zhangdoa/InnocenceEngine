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
	BaseActor testLightActor1;
	BaseActor testLightActor2;
	BaseActor testLightActor3;

	BaseActor testStaticMeshActor1;
	BaseActor testStaticMeshActor2;

	VisibleComponent testSkyboxComponent;

	LightComponent testLightComponent1;
	LightComponent testLightComponent2;
	LightComponent testLightComponent3;

	VisibleComponent testLightBillboardComponent1;
	VisibleComponent testLightBillboardComponent2;
	VisibleComponent testLightBillboardComponent3;

	VisibleComponent testStaticMeshComponent1;
	VisibleComponent testStaticMeshComponent2;

	double temp = 0.0f;
};

