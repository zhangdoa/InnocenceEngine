#pragma once
#include "manager/BaseManager.h"
#include "manager/LogManager.h"
#include "entity/InnoMath.h"

class GLWindowManager : public BaseManager
{
public:
	~GLWindowManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static GLWindowManager& getInstance()
	{
		static GLWindowManager instance;
		return instance;
	}

	GLFWwindow* getWindow() const;
	vec2 getScreenCenterPosition() const;
	vec2 getScreenResolution() const;
	void setWindowName(const std::string& windowName);
	void hideMouseCursor() const;
	void showMouseCursor() const;

private:
	GLWindowManager() {};

	unsigned int SCR_WIDTH = 1280;
	unsigned int SCR_HEIGHT = 720;
	GLFWwindow* m_window;
	std::string m_windowName;
};

