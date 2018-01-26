#pragma once
#include "../../interface/IManager.h"
#include "../../manager/LogManager.h"
#include "../../data/InnoMath.h"

class GLWindowManager : public IManager
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

	const unsigned int SCR_WIDTH = 1024;
	const unsigned int SCR_HEIGHT = 768;
	GLFWwindow* m_window;
	std::string m_windowName;
};

