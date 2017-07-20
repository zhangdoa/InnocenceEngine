#include "stdafx.h"
#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}


InnocenceGarden::~InnocenceGarden()
{
}

const std::string & InnocenceGarden::getGameName()
{
	return m_gameName;
}

CameraComponent * InnocenceGarden::getCameraComponent()
{
	return &testCameraComponent;
}

IVisibleGameEntity * InnocenceGarden::getSkybox()
{
	return &testSkyboxComponent;
}

IVisibleGameEntity * InnocenceGarden::getTest()
{
	return &testTriangleComponent;
}

void InnocenceGarden::init()
{
	testRootActor.addChildActor(&testCameraActor);
	testRootActor.addChildActor(&testSkyboxActor);
	testRootActor.addChildActor(&testTriangleActor);

	testCameraActor.addChildComponent(&testCameraComponent);
	testSkyboxActor.addChildComponent(&testSkyboxComponent);
	testTriangleActor.addChildComponent(&testTriangleComponent);
	testRootActor.exec(INIT);
	testTriangleActor.getTransform()->setPos(glm::vec3(0.0f, 0.0f, -5.0f));
}

void InnocenceGarden::update()
{
	testTriangleActor.getTransform()->rotate(glm::vec3(-1.0f, 0.0f, 0.0f), 1.0);
	testTriangleActor.getTransform()->rotate(glm::vec3(0.0f, -1.0f, 0.0f), 0.5);
	testTriangleActor.getTransform()->rotate(glm::vec3(0.0f, 0.0f, -1.0f), 0.25);
	testRootActor.exec(UPDATE);	
}

void InnocenceGarden::shutdown()
{	
	testRootActor.exec(SHUTDOWN);
}
