#pragma once
#include "IEventManager.h"
class UIManager : public IEventManager
{
public:
	UIManager();
	~UIManager();

private:
	void init() override;
	void update() override;
	void shutdown() override;

	GLFWwindow* m_window;
};

