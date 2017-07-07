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
	testTriangleActor.getTransform()->setPos(Vec3f(0.0f, -1.0f, 5.0f));
}

void InnocenceGarden::update()
{
	testRootActor.exec(UPDATE);
	//testTriangle.getTransform()->setPos(testTriangle.getTransform()->getPos() + Vec3f(1.0f, 0.0f, 0.0f));
}

void InnocenceGarden::shutdown()
{	
	testRootActor.exec(SHUTDOWN);
}
