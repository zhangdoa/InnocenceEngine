#pragma once
#include "Math.h"
#include "IVisibleGameEntity.h"
#include "IEventManager.h"
#include "LogManager.h"
#include "GLRenderingManager.h"
#include "CameraComponent.h"


class GraphicManager : public IEventManager
{
public:
	GraphicManager();
	~GraphicManager();

	void render(IVisibleGameEntity* visibleGameEntity);
	void setCameraViewProjectionMatrix(const Mat4f& cameraViewProjectionMatrix);
private:
	void init() override;
	void update() override;
	void shutdown() override;

	GLRenderingManager m_renderingManager;
};

