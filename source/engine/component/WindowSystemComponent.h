#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "../common/InnoConcurrency.h"

class WindowSystemComponent
{
public:
	~WindowSystemComponent() {};
	WindowSystemComponent(const WindowSystemComponent&) = delete;
	WindowSystemComponent& operator=(const WindowSystemComponent&) = delete;

	static WindowSystemComponent& get()
	{
		static WindowSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	//window data
	TVec2<unsigned int> m_windowResolution = TVec2<unsigned int>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

private:
	WindowSystemComponent() {};
};
