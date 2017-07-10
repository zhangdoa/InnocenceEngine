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
	testTriangleActor.getTransform()->setPos(glm::vec3(0.0f, 0.5f, 0.0f));
	testCameraActor.getTransform()->rotate(glm::vec3(0.0f, 1.0f, 0.0f), 45);
}

void InnocenceGarden::update()
{
	testRootActor.exec(UPDATE);	
}

void InnocenceGarden::shutdown()
{	
	testRootActor.exec(SHUTDOWN);
}
