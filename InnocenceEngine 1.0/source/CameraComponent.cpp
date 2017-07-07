#include "stdafx.h"
#include "CameraComponent.h"

CameraData::CameraData()
{
}

CameraData::~CameraData()
{
}

void CameraData::addCameraData(float fov, float aspectRatio, float zNear, float zFar)
{
	m_projectionMatrix.initPerspective(fov, aspectRatio, zNear, zFar);
}

Mat4f CameraData::getViewProjectionMatrix(BaseComponent* parent)
{
	Mat4f l_cameraRotationMatrix = parent->getParentActor()->caclTransformedRot().getConjugatedVec4f().toRotationMatrix();

	Vec3f l_cameraPosition = parent->getParentActor()->caclTransformedPos() * -1.0f;

	Mat4f l_cameraTranslationMatrix;
	l_cameraTranslationMatrix.initTranslation(l_cameraPosition.getX(), l_cameraPosition.getY(),
		l_cameraPosition.getZ());
	return m_projectionMatrix * (l_cameraRotationMatrix * l_cameraTranslationMatrix);
}

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

Mat4f CameraComponent::getViewProjectionMatrix()
{
	return m_cameraData.getViewProjectionMatrix(this);
}

void CameraComponent::move(moveDirection moveDirection)
{
	switch (moveDirection)
	{
	case FORWARD:  this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRot().getForward() * 3); break;
	case BACKWARD:  this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRot().getForward() * -3);  break;
	case LEFT:   this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRot().getRight() * -3);  break;
	case RIGHT:   this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRot().getRight() * 3);  break;
	}

}

void CameraComponent::init()
{
	m_cameraData.addCameraData((70.0f / 180.0f) * PI, (16.0f / 9.0f), 0.1f, 1000.0f);
}

void CameraComponent::update()
{
	getTransform()->update();
	if (InputManager::getInstance().getMouse(GLFW_MOUSE_BUTTON_RIGHT))
	{
		Vec2f deltaPos = InputManager::getInstance().getMousePosition() - WindowManager::getInstance().getScreenCenterPosition();
		if (deltaPos.getX() != 0)
		{
			getTransform()->rotate(Vec3f(0, 1, 0), ((deltaPos.getX() * 0.01f) / 180.0f)* PI);
		}
		if (deltaPos.getY() != 0)
		{
			getTransform()->rotate(getTransform()->getRot().getRight(), ((-deltaPos.getY() * 0.01f) / 180.0f)* PI);
		}
		if (deltaPos.getX() != 0 || deltaPos.getY() != 0)
		{
			InputManager::getInstance().setMousePosition(WindowManager::getInstance().getScreenCenterPosition());
		}

		if (InputManager::getInstance().getKey(GLFW_KEY_W))
		{
			move(CameraComponent::FORWARD);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_S))
		{
			move(CameraComponent::BACKWARD);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_A))
		{
			move(CameraComponent::LEFT);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_D))
		{
			move(CameraComponent::RIGHT);
		}
	}
}

void CameraComponent::shutdown()
{
}

