#pragma once
#include "../common/InnoType.h"

class InputComponent
{
public:
	InputComponent() {};
	~InputComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;
	unsigned int m_UUID = 0;

	ButtonStatusCallbackMap m_buttonStatusCallbackImpl;
	MouseMovementCallbackMap m_mouseMovementCallbackImpl;
};
