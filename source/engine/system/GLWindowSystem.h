#pragma once
#include "interface/ISystem.h"
#include "interface/ILogSystem.h"
#include "entity/InnoMath.h"

extern ILogSystem* g_pLogSystem;

class GLWindowSystem : public ISystem
{
public:
	~GLWindowSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	static GLWindowSystem& getInstance()
	{
		static GLWindowSystem instance;
		return instance;
	}

	GLFWwindow* getWindow() const;
	vec2 getScreenCenterPosition() const;
	vec2 getScreenResolution() const;
	void setWindowName(const std::string& windowName);
	void hideMouseCursor() const;
	void showMouseCursor() const;

private:
	GLWindowSystem() {};
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	unsigned int SCR_WIDTH = 1280;
	unsigned int SCR_HEIGHT = 720;
	GLFWwindow* m_window;
	std::string m_windowName;
};

