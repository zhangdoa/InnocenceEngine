#pragma once
#include "../common/InnoType.h"

class InputComponent
{
public:
	InputComponent() {};
	~InputComponent() {};
	
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	buttonStatusCallbackMap m_buttonStatusCallbackImpl;
	mouseMovementCallbackMap m_mouseMovementCallbackImpl;
};
