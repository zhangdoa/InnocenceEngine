#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "../common/InnoConcurrency.h"

class WindowSystemSingletonComponent
{
public:
	~WindowSystemSingletonComponent() {};
	WindowSystemSingletonComponent(const WindowSystemSingletonComponent&) = delete;
	WindowSystemSingletonComponent& operator=(const WindowSystemSingletonComponent&) = delete;

	static WindowSystemSingletonComponent& getInstance()
	{
		static WindowSystemSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	//window data
	vec2 m_windowResolution = vec2(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

	//input data
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;

	buttonStatusMap m_buttonStatus;
	buttonStatusCallbackMap m_buttonStatusCallback;
	mouseMovementCallbackMap m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;

	vec4 m_mousePositionInWorldSpace;
	Ray m_mouseRay;

private:
	WindowSystemSingletonComponent() {};
};
