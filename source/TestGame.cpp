#include "TestGame.h"



TestGame::TestGame()
{
}


TestGame::~TestGame()
{
}

void TestGame::init()
{
	testMeshObject = new GameObject();
	testMeshComponent = new StaticMeshComponent();
	this->addObject(testMeshObject);
	testMeshObject->addComponent(testMeshComponent);
}