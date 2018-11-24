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

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	//window data
	TVec2<unsigned int> m_windowResolution = TVec2<unsigned int>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

	//input data
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;

	ButtonStatusMap m_buttonStatus;
	ButtonStatusCallbackMap m_buttonStatusCallback;
	MouseMovementCallbackMap m_mouseMovementCallback;

	float m_mouseXOffset;
	float m_mouseYOffset;
	float m_mouseLastX;
	float m_mouseLastY;

	vec4 m_mousePositionInWorldSpace;
private:
	WindowSystemSingletonComponent() {};
};
