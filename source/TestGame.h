#pragma once
#include "Game.h"
#include "StaticMeshComponent.h"
class TestGame : public Game
{
public:
	TestGame();
	~TestGame();
	void init();
private:
	StaticMeshComponent* testMeshComponent;
	GameObject* testMeshObject;
};

