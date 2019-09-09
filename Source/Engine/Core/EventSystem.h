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

	void addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent) override;
	void addMouseMovementCallback(int32_t mouseCode, std::function<void(float)>* mouseMovementCallback) override;

	void buttonStatusCallback(ButtonState buttonState) override;
	void framebufferSizeCallback(int32_t width, int32_t height) override;
	void mousePositionCallback(float mouseXPos, float mouseYPos) override;
	void scrollCallback(float xoffset, float yoffset) override;

	Vec4 getMousePositionInWorldSpace() override;
	Vec2 getMousePositionInScreenSpace() override;

	ObjectStatus getStatus() override;
};