#pragma once
#include "IGameData.h"
#include "StaticMeshComponent.h"
class InnocenceGarden :	public IGameData
{
public:
	InnocenceGarden();
	~InnocenceGarden();

private:
	void init() override;
	void update() override;
	void shutdown() override;

	StaticMeshComponent testTriangle;
};

