#pragma once
#include "../Common/InnoComponent.h"

class InputComponent : public InnoComponent
{
public:
	InputComponent() {};
	~InputComponent() {};

	ButtonStatusCallbackMap m_buttonStatusCallbackImpl;
	MouseMovementCallbackMap m_mouseMovementCallbackImpl;
};
