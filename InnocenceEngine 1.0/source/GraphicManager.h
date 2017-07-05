#pragma once
#include "Math.h"
#include "IEventManager.h"
#include "LogManager.h"
#include "UIManager.h"
#include "GLRenderingManager.h"
#include "StaticMeshComponent.h"


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

