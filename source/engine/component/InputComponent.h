#pragma once
#include "BaseComponent.h"

class InputComponent : public BaseComponent
{
public:
	InputComponent() {};
	~InputComponent() {};
	
	std::unordered_map<int, std::vector<std::function<void()>*>> m_keyboardInputCallbackImpl;
	std::unordered_map<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallbackImpl;
};
