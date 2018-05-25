#include "InputComponent.h"


InputComponent::InputComponent()
{
}

InputComponent::~InputComponent()
{
}

void InputComponent::registerKeyboardInputCallback(int keyCode, std::function<void()>* function)
{
	auto l_keyboardInputCallbackVector = m_keyboardInputCallbackImpl.find(keyCode);
	if (l_keyboardInputCallbackVector != m_keyboardInputCallbackImpl.end())
	{
		l_keyboardInputCallbackVector->second.emplace_back(function);
	}
	else
	{
		m_keyboardInputCallbackImpl.emplace(keyCode, std::vector<std::function<void()>*>{function});
	}
}

void InputComponent::registerMouseInputCallback(int mouseCode, std::function<void(double)>* function)
{
	auto l_mouseInputCallbackVector = m_mouseMovementCallbackImpl.find(mouseCode);
	if (l_mouseInputCallbackVector != m_mouseMovementCallbackImpl.end())
	{
		l_mouseInputCallbackVector->second.emplace_back(function);
	}
	else
	{
		m_mouseMovementCallbackImpl.emplace(mouseCode, std::vector<std::function<void(double)>*>{function});
	}
}

std::unordered_map<int, std::vector<std::function<void()>*>>& InputComponent::getKeyboardInputCallbackContainer()
{
	return m_keyboardInputCallbackImpl;
}

std::unordered_map<int, std::vector<std::function<void(double)>*>>& InputComponent::getMouseInputCallbackContainer()
{
	return m_mouseMovementCallbackImpl;
}

void InputComponent::setup()
{
}

void InputComponent::initialize()
{
}


void InputComponent::shutdown()
{
}

