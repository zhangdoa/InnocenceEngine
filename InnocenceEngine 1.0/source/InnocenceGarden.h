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

	CameraComponent* getCameraComponent();
	IVisibleGameEntity* getSkybox();
	IVisibleGameEntity* getTestCube();

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

