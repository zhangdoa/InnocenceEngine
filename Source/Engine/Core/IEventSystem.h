#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoMathHelper.h"

#include "../Common/InnoClassTemplate.h"

struct InputConfig
{
	int32_t totalKeyCodes;
	int32_t totalMouseCodes;
};

class IEventSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IEventSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual InputConfig getInputConfig() = 0;

	virtual void addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent) = 0;
	virtual void addMouseMovementCallback(int32_t mouseCode, std::function<void(float)>* mouseMovementCallback) = 0;

	virtual void buttonStatusCallback(ButtonState buttonState) = 0;
	virtual void framebufferSizeCallback(int32_t width, int32_t height) = 0;
	virtual void mousePositionCallback(float mouseXPos, float mouseYPos) = 0;
	virtual void scrollCallback(float xoffset, float yoffset) = 0;

	virtual Vec4 getMousePositionInWorldSpace() = 0;
	virtual Vec2 getMousePositionInScreenSpace() = 0;

	virtual ObjectStatus getStatus() = 0;
};
