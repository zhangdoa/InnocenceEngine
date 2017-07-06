#pragma once
#include "IEventManager.h"
#include "LogManager.h"

class WindowManager : public IEventManager
{
public:
	WindowManager();
	~WindowManager();
	GLFWwindow* getWindow();

private:
	void init() override;
	void update() override;
	void shutdown() override;

	GLFWwindow* m_window;
};

