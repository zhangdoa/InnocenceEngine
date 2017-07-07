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
	return &testCamera;
}

IVisibleGameEntity * InnocenceGarden::getTest()
{
	return &testTriangle;
}

void InnocenceGarden::init()
{
	testCamera.exec(INIT);
	testTriangle.exec(INIT);
	testCamera.getTransform()->setPos(Vec3f(0.0f, 1.0f, -5.0f));
}

void InnocenceGarden::update()
{
	if (getInputManager()->getKey(GLFW_KEY_W))
	{
		testCamera.move(CameraComponent::FORWARD);
	}
	if (getInputManager()->getKey(GLFW_KEY_S))
	{
		testCamera.move(CameraComponent::BACKWARD);
	}
	if (getInputManager()->getKey(GLFW_KEY_A))
	{
		testCamera.move(CameraComponent::LEFT);
	}
	if (getInputManager()->getKey(GLFW_KEY_D))
	{
		testCamera.move(CameraComponent::RIGHT);
	}
	testCamera.exec(UPDATE);
	testTriangle.exec(UPDATE);
}

void InnocenceGarden::shutdown()
{	
	testCamera.exec(SHUTDOWN);
	testTriangle.exec(SHUTDOWN);
}
