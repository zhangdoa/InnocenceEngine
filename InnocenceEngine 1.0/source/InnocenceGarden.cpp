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
	testCamera.getTransform()->setPos(Vec3f(5.0f, 1.0f, -15.0f));
}

void InnocenceGarden::update()
{
	if (InputManager::getInstance().getMouse(GLFW_MOUSE_BUTTON_RIGHT))
	{
		Vec2f deltaPos = InputManager::getInstance().getMousePosition() - WindowManager::getInstance().getScreenCenterPosition();
		if (deltaPos.getX() != 0)
		{
			testCamera.getTransform()->rotate(*testCamera.getYAxis(), ((deltaPos.getX() * 0.01f) / 180.0f)* PI);
		}
		if (deltaPos.getY() != 0)
		{
			testCamera.getTransform()->rotate(testCamera.getTransform()->getRot().getRight(), ((-deltaPos.getY() * 0.01f) / 180.0f)* PI);
		}
		if (deltaPos.getX() != 0 || deltaPos.getY() != 0)
		{
			InputManager::getInstance().setMousePosition(WindowManager::getInstance().getScreenCenterPosition());
		}


		if (InputManager::getInstance().getKey(GLFW_KEY_W))
		{
			testCamera.move(CameraComponent::FORWARD);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_S))
		{
			testCamera.move(CameraComponent::BACKWARD);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_A))
		{
			testCamera.move(CameraComponent::LEFT);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_D))
		{
			testCamera.move(CameraComponent::RIGHT);
		}
	}
	testCamera.exec(UPDATE);
	testTriangle.exec(UPDATE);
}

void InnocenceGarden::shutdown()
{	
	testCamera.exec(SHUTDOWN);
	testTriangle.exec(SHUTDOWN);
}
