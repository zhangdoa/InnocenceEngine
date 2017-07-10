#pragma once
#include "IEventManager.h"
#include "LogManager.h"

class WindowManager : public IEventManager
{
public:
	WindowManager();
	~WindowManager();

	static WindowManager& getInstance()
	{
		static WindowManager instance;
		return instance;
	}

	GLFWwindow* getWindow() const;
	glm::vec2 getScreenCenterPosition() const;

private:
	const unsigned int SCR_WIDTH = 1024;
	const unsigned int SCR_HEIGHT = 768;
	void init() override;
	void update() override;
	void shutdown() override;

	GLFWwindow* m_window;
};

