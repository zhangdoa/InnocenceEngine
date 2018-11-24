#pragma once
#include "IInputSystem.h"

class InnoInputSystem : INNO_IMPLEMENT IInputSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoInputSystem);

	INNO_SYSTEM_EXPORT void setup() override;
	INNO_SYSTEM_EXPORT void initialize() override;
	INNO_SYSTEM_EXPORT void update() override;
	INNO_SYSTEM_EXPORT void terminate() override;

	INNO_SYSTEM_EXPORT void addButtonStatusCallback(ButtonData boundButton, std::function<void()>* buttonStatusCallbackFunctor) override;
	INNO_SYSTEM_EXPORT void addButtonStatusCallback(ButtonData boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor) override;
	INNO_SYSTEM_EXPORT void addButtonStatusCallback(ButtonStatusCallbackMap & buttonStatusCallbackFunctor) override;
	INNO_SYSTEM_EXPORT void addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback) override;
	INNO_SYSTEM_EXPORT void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(float)>*>& mouseMovementCallback) override;
	INNO_SYSTEM_EXPORT void addMouseMovementCallback(MouseMovementCallbackMap& mouseMovementCallback) override;

	INNO_SYSTEM_EXPORT void framebufferSizeCallback(int width, int height) override;
	INNO_SYSTEM_EXPORT void mousePositionCallback(float mouseXPos, float mouseYPos) override;
	INNO_SYSTEM_EXPORT void scrollCallback(float xoffset, float yoffset) override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;
};