#pragma once
#include "BaseComponent.h"

class InputComponent : public BaseComponent
{
public:
	InputComponent() {};
	~InputComponent() {};
	
	buttonStatusCallbackMap m_buttonStatusCallbackImpl;
	mouseMovementCallbackMap m_mouseMovementCallbackImpl;
};
