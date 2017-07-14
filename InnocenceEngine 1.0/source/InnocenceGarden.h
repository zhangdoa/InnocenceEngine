#pragma once
#include "IGameData.h"
#include "IGameEntity.h"
#include "CameraComponent.h"
#include "SkyboxComponent.h"
#include "StaticMeshComponent.h"
class InnocenceGarden :	public IGameData
{
public:
	InnocenceGarden();
	~InnocenceGarden();

	const std::string& getGameName() override;
	CameraComponent* getCameraComponent() override;
	IVisibleGameEntity* getSkybox() override;
	IVisibleGameEntity* getTest() override;

private:
	void init() override;
	void update() override;
	void shutdown() override;

	const std::string m_gameName = "Innocence Garden";
	BaseActor testRootActor;

	BaseActor testCameraActor;
	BaseActor testSkyboxActor;
	BaseActor testTriangleActor;

	CameraComponent testCameraComponent;
	SkyboxComponent testSkyboxComponent;
	StaticMeshComponent testTriangleComponent;
};

