#include "InputComponent.h"


InputComponent::InputComponent()
{
}

InputComponent::~InputComponent()
{
}

template<typename T>
inline void InputComponent::registerInputCallback(int keyCode, void* function, T * owner)
{
	m_keyboardInputCallbackImpl.emplace(keyCode, std::vector<std::function<void()>*>{std::bind(&function, owner)});
}

std::multimap<int, std::vector<std::function<void()>*>>& InputComponent::getKeyboardInputCallbackImpl()
{
	return m_keyboardInputCallbackImpl;
}

std::multimap<int, std::vector<std::function<void(double)>*>>& InputComponent::getMouseInputCallbackImpl()
{
	return m_mouseMovementCallbackImpl;
}

void InputComponent::rotateAroundPositiveYAxis(double offset)
{
	getParentEntity()->getTransform()->rotateInLocal(vec4(0.0, 1.0, 0.0, 0.0), ((-offset * rotateSpeed) / 180.0)* PI);
}

void InputComponent::rotateAroundRightAxis(double offset)
{
	getParentEntity()->getTransform()->rotateInLocal(getParentEntity()->getTransform()->getDirection(direction::RIGHT), ((offset * rotateSpeed) / 180.0)* PI);
}

void InputComponent::setup()
{
	f_rotateAroundPositiveYAxis = std::bind(&InputComponent::rotateAroundPositiveYAxis, this, std::placeholders::_1);
	f_rotateAroundRightAxis = std::bind(&InputComponent::rotateAroundRightAxis, this, std::placeholders::_1);

	// @TODO: key name binding
	m_mouseMovementCallbackImpl.emplace(0, std::vector<std::function<void(double)>*>{&f_rotateAroundPositiveYAxis});
	m_mouseMovementCallbackImpl.emplace(1, std::vector<std::function<void(double)>*>{&f_rotateAroundRightAxis});
}

void InputComponent::initialize()
{
}


void InputComponent::shutdown()
{
}

