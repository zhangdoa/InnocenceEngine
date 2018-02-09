#include "../../main/stdafx.h"
#include "InputComponent.h"


InputComponent::InputComponent()
{
}


InputComponent::~InputComponent()
{
}


void InputComponent::registerInputCallback(int keyCode, void * function)
{
	// @TODO: it seems that there should be a template for unknown member function 
}

std::multimap<int, std::vector<std::function<void()>*>>& InputComponent::getKeyboardInputCallbackImpl()
{
	return m_keyboardInputCallbackImpl;
}

std::multimap<int, std::vector<std::function<void(float)>*>>& InputComponent::getMouseInputCallbackImpl()
{
	return m_mouseMovementCallbackImpl;
}

void InputComponent::move(moveDirection moveDirection)
{
	switch (moveDirection)
	{
		// opengl use right-hand-coordinate, so go foward means get into the negative z-axis
	case FORWARD:  getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::BACKWARD).mul(moveSpeed)); break;
	case BACKWARD:  getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::FORWARD).mul(moveSpeed));  break;
	case LEFT:   getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::LEFT).mul(moveSpeed));  break;
	case RIGHT:   getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::RIGHT).mul(moveSpeed));  break;
	}
}

void InputComponent::rotateAroundXAxis(float offset)
{
	getTransform()->rotate(vec3(0.0f, 1.0f, 0.0f), ((-offset * rotateSpeed) / 180.0f)* PI);
}

void InputComponent::rotateAroundYAxis(float offset)
{
	getTransform()->rotate(getTransform()->getDirection(Transform::RIGHT), ((offset * rotateSpeed) / 180.0f)* PI);
}

void InputComponent::setup()
{
	BaseEntity::setup();

	f_moveForward = std::bind(&InputComponent::moveForward, this);
	f_moveBackward = std::bind(&InputComponent::moveBackward, this);
	f_moveLeft = std::bind(&InputComponent::moveLeft, this);
	f_moveRight = std::bind(&InputComponent::moveRight, this);
	f_rotateAroundXAxis = std::bind(&InputComponent::rotateAroundXAxis, this, std::placeholders::_1);
	f_rotateAroundYAxis = std::bind(&InputComponent::rotateAroundYAxis, this, std::placeholders::_1);

	m_keyboardInputCallbackImpl.emplace(GLFW_KEY_W, std::vector<std::function<void()>*>{&f_moveForward});
	m_keyboardInputCallbackImpl.emplace(GLFW_KEY_S, std::vector<std::function<void()>*>{&f_moveBackward});
	m_keyboardInputCallbackImpl.emplace(GLFW_KEY_A, std::vector<std::function<void()>*>{&f_moveLeft});
	m_keyboardInputCallbackImpl.emplace(GLFW_KEY_D, std::vector<std::function<void()>*>{&f_moveRight});

	// @TODO: key name binding
	m_mouseMovementCallbackImpl.emplace(0, std::vector<std::function<void(float)>*>{&f_rotateAroundXAxis});
	m_mouseMovementCallbackImpl.emplace(1, std::vector<std::function<void(float)>*>{&f_rotateAroundYAxis});
}

void InputComponent::initialize()
{
}

void InputComponent::update()
{
}

void InputComponent::shutdown()
{
}

void InputComponent::moveForward()
{
	move(moveDirection::FORWARD);
}

void InputComponent::moveBackward()
{
	move(moveDirection::BACKWARD);
}

void InputComponent::moveLeft()
{
	move(moveDirection::LEFT);
}

void InputComponent::moveRight()
{
	move(moveDirection::RIGHT);
}

