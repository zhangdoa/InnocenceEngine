#pragma once
#include "IEventManager.h"
#include "LogManager.h"

class WindowManager : public IEventManager
{
public:
	~WindowManager();

	static WindowManager& getInstance()
	{
		static WindowManager instance;
		return instance;
	}

	GLFWwindow* getWindow() const;
	glm::vec2 getScreenCenterPosition() const;
	void setWindowName(const std::string& windowName);

private:
	WindowManager();

	std::string m_windowName;
	void init() override;
	void update() override;
	void shutdown() override;

	const unsigned int SCR_WIDTH = 1024;
	const unsigned int SCR_HEIGHT = 768;
	GLFWwindow* m_window;
};

