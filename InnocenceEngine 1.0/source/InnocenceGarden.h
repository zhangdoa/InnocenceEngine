#pragma once
#include "IGameData.h"
#include "CameraComponent.h"
#include "StaticMeshComponent.h"
class InnocenceGarden :	public IGameData
{
public:
	InnocenceGarden();
	~InnocenceGarden();

	CameraComponent* getCameraComponent() override;

private:
	void init() override;
	void update() override;
	void shutdown() override;

	CameraComponent testCamera;
	StaticMeshComponent testTriangle;
};

