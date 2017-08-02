#pragma once
#include "../core/interface/IGameData.h"
#include "../core/interface/IGameEntity.h"
#include "../core/component/CameraComponent.h"
#include "../core/component/SkyboxComponent.h"
#include "../core/component/StaticMeshComponent.h"
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

