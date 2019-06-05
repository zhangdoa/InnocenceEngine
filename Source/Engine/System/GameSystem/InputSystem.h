#pragma once
#include "IInputSystem.h"

INNO_CONCRETE InnoInputSystem : INNO_IMPLEMENT IInputSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoInputSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	InputConfig getInputConfig() override;

	void addButtonStatusCallback(ButtonData boundButton, std::function<void()>* buttonStatusCallbackFunctor) override;
	void addButtonStatusCallback(ButtonData boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor) override;
	void addButtonStatusCallback(ButtonStatusCallbackMap & buttonStatusCallbackFunctor) override;
	void addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback) override;
	void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(float)>*>& mouseMovementCallback) override;
	void addMouseMovementCallback(MouseMovementCallbackMap& mouseMovementCallback) override;

	void framebufferSizeCallback(int width, int height) override;
	void mousePositionCallback(float mouseXPos, float mouseYPos) override;
	void scrollCallback(float xoffset, float yoffset) override;

	vec4 getMousePositionInWorldSpace() override;
	vec2 getMousePositionInScreenSpace() override;

	ObjectStatus getStatus() override;
};