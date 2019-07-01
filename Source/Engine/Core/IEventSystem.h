#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoMath.h"

#include "../Common/InnoClassTemplate.h"

struct InputConfig
{
	int totalKeyCodes;
	int totalMouseCodes;
};

INNO_INTERFACE IEventSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IEventSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual InputConfig getInputConfig() = 0;

	virtual void addButtonStatusCallback(ButtonData boundButton, std::function<void()>* buttonStatusCallbackFunctor) = 0;
	virtual void addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback) = 0;

	virtual void buttonStatusCallback(ButtonData boundButton) = 0;
	virtual void framebufferSizeCallback(int width, int height) = 0;
	virtual void mousePositionCallback(float mouseXPos, float mouseYPos) = 0;
	virtual void scrollCallback(float xoffset, float yoffset) = 0;

	virtual vec4 getMousePositionInWorldSpace() = 0;
	virtual vec2 getMousePositionInScreenSpace() = 0;

	virtual ObjectStatus getStatus() = 0;
};
