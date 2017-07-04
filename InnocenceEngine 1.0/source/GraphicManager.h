#pragma once
#include "IEventManager.h"
#include "GLRenderingManager.h"

class GraphicManager : public IEventManager
{
public:
	GraphicManager();
	~GraphicManager();
	GLFWwindow* getWindow();

private:
	void init() override;
	void update() override;
	void shutdown() override;

	GLFWwindow* m_window;
	GLRenderingManager m_renderingManager;
};

