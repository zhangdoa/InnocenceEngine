#pragma once
#include "IEventSystem.h"

class InnoEventSystem : public IEventSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoEventSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	InputConfig getInputConfig() override;

	void addButtonStatusCallback(ButtonData boundButton, std::function<void()>* buttonStatusCallbackFunctor) override;
	void addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback) override;

	void buttonStatusCallback(ButtonData boundButton) override;
	void framebufferSizeCallback(int width, int height) override;
	void mousePositionCallback(float mouseXPos, float mouseYPos) override;
	void scrollCallback(float xoffset, float yoffset) override;

	vec4 getMousePositionInWorldSpace() override;
	vec2 getMousePositionInScreenSpace() override;

	ObjectStatus getStatus() override;
};