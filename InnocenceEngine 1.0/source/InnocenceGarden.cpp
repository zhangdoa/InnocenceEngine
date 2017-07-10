#include "stdafx.h"
#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}


InnocenceGarden::~InnocenceGarden()
{
}

CameraComponent * InnocenceGarden::getCameraComponent()
{
	return &testCameraComponent;
}

IVisibleGameEntity * InnocenceGarden::getTest()
{
	return &testTriangleComponent;
}

void InnocenceGarden::init()
{
	testRootActor.addChildActor(&testCameraActor);
	testRootActor.addChildActor(&testTriangleActor);
	testCameraActor.addChildComponent(&testCameraComponent);
	testTriangleActor.addChildComponent(&testTriangleComponent);
	testRootActor.exec(INIT);
}

void InnocenceGarden::update()
{
	testRootActor.exec(UPDATE);
	//LogManager::printLog(testTriangleActor.getTransform()->getPos());
	//testTriangleActor.getTransform()->setPos(testTriangleActor.getTransform()->getPos() + glm::vec3(1.0f, 0.0f, 0.0f));
}

void InnocenceGarden::shutdown()
{	
	testRootActor.exec(SHUTDOWN);
}
