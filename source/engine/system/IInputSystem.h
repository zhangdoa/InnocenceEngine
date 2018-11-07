#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

INNO_INTERFACE IInputSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IInputSystem);

	INNO_SYSTEM_EXPORT virtual void setup() = 0;
	INNO_SYSTEM_EXPORT virtual void initialize() = 0;
	INNO_SYSTEM_EXPORT virtual void update() = 0;
	INNO_SYSTEM_EXPORT virtual void terminate() = 0;

	INNO_SYSTEM_EXPORT virtual void addButtonStatusCallback(button boundButton, std::function<void()>* buttonStatusCallbackFunctor) = 0;
	INNO_SYSTEM_EXPORT virtual void addButtonStatusCallback(button boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor) = 0;
	INNO_SYSTEM_EXPORT virtual void addButtonStatusCallback(buttonStatusCallbackMap & buttonStatusCallbackFunctor) = 0;
	INNO_SYSTEM_EXPORT virtual void addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback) = 0;
	INNO_SYSTEM_EXPORT virtual void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(float)>*>& mouseMovementCallback) = 0;
	INNO_SYSTEM_EXPORT virtual void addMouseMovementCallback(mouseMovementCallbackMap& mouseMovementCallback) = 0;

	INNO_SYSTEM_EXPORT virtual void framebufferSizeCallback(int width, int height) = 0;
	INNO_SYSTEM_EXPORT virtual void mousePositionCallback(float mouseXPos, float mouseYPos) = 0;
	INNO_SYSTEM_EXPORT virtual void scrollCallback(float xoffset, float yoffset) = 0;

	INNO_SYSTEM_EXPORT virtual objectStatus getStatus() = 0;
};
