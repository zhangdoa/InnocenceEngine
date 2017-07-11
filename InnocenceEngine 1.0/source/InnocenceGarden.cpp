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
	testTriangleActor.getTransform()->setPos(glm::vec3(0.0f, 0.0f, -20.0f));
	//testTriangleActor.getTransform()->rotate(glm::vec3(1.0f, 0.0f, 0.0f), 45);
	testCameraActor.getTransform()->rotate(glm::vec3(0.0f, 1.0f, 0.0f), 30);
	testCameraComponent.move(CameraComponent::FORWARD);
}

void InnocenceGarden::update()
{
	LogManager::getInstance().printLog(testCameraActor.getTransform()->getDirection(Transform::FORWARD));
	//LogManager::getInstance().printLog(testCameraActor.getTransform()->QuatToRotationMatrix(testCameraActor.getTransform()->getRot()));
	testRootActor.exec(UPDATE);	
}

void InnocenceGarden::shutdown()
{	
	testRootActor.exec(SHUTDOWN);
}
