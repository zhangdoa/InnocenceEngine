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

std::multimap<int, std::vector<std::function<void(double)>*>>& InputComponent::getMouseInputCallbackImpl()
{
	return m_mouseMovementCallbackImpl;
}

void InputComponent::move(moveDirection moveDirection)
{
	switch (moveDirection)
	{
	case FORWARD:  getParentEntity()->getTransform()->setPos(getParentEntity()->getTransform()->getPos() + getParentEntity()->getTransform()->getDirection(Transform::FORWARD).scale(moveSpeed)); break;
	case BACKWARD:  getParentEntity()->getTransform()->setPos(getParentEntity()->getTransform()->getPos() + getParentEntity()->getTransform()->getDirection(Transform::BACKWARD).scale(moveSpeed));  break;
	case LEFT:   getParentEntity()->getTransform()->setPos(getParentEntity()->getTransform()->getPos() + getParentEntity()->getTransform()->getDirection(Transform::LEFT).scale(moveSpeed));  break;
	case RIGHT:   getParentEntity()->getTransform()->setPos(getParentEntity()->getTransform()->getPos() + getParentEntity()->getTransform()->getDirection(Transform::RIGHT).scale(moveSpeed));  break;
	}
}

void InputComponent::rotateAroundPositiveYAxis(double offset)
{
	getParentEntity()->getTransform()->rotate(vec4(0.0, 1.0, 0.0, 0.0), ((-offset * rotateSpeed) / 180.0)* PI);
}

void InputComponent::rotateAroundRightAxis(double offset)
{
	getParentEntity()->getTransform()->rotate(getParentEntity()->getTransform()->getDirection(Transform::RIGHT), ((offset * rotateSpeed) / 180.0)* PI);
}

void InputComponent::setup()
{
	f_moveForward = std::bind(&InputComponent::moveForward, this);
	f_moveBackward = std::bind(&InputComponent::moveBackward, this);
	f_moveLeft = std::bind(&InputComponent::moveLeft, this);
	f_moveRight = std::bind(&InputComponent::moveRight, this);
	f_rotateAroundPositiveYAxis = std::bind(&InputComponent::rotateAroundPositiveYAxis, this, std::placeholders::_1);
	f_rotateAroundRightAxis = std::bind(&InputComponent::rotateAroundRightAxis, this, std::placeholders::_1);

	m_keyboardInputCallbackImpl.emplace(INNO_KEY_S, std::vector<std::function<void()>*>{&f_moveForward});
	m_keyboardInputCallbackImpl.emplace(INNO_KEY_W, std::vector<std::function<void()>*>{&f_moveBackward});
	m_keyboardInputCallbackImpl.emplace(INNO_KEY_A, std::vector<std::function<void()>*>{&f_moveLeft});
	m_keyboardInputCallbackImpl.emplace(INNO_KEY_D, std::vector<std::function<void()>*>{&f_moveRight});

	// @TODO: key name binding
	m_mouseMovementCallbackImpl.emplace(0, std::vector<std::function<void(double)>*>{&f_rotateAroundPositiveYAxis});
	m_mouseMovementCallbackImpl.emplace(1, std::vector<std::function<void(double)>*>{&f_rotateAroundRightAxis});
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

