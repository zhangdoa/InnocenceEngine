#pragma once
#include "IEventManager.h"
#include "UIManager.h"
#include "GLRenderingManager.h"

class GraphicManager : public IEventManager
{
public:
	GraphicManager();
	~GraphicManager();

private:
	void init() override;
	void update() override;
	void shutdown() override;

	UIManager m_uiManager;
	GLRenderingManager m_renderingManager;

};

