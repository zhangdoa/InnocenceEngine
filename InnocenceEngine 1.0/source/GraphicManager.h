#pragma once
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

	static GraphicManager& getInstance()
	{
		static GraphicManager instance;
		return instance;
	}

	void render(IVisibleGameEntity* visibleGameEntity) const;
	void setCameraViewProjectionMatrix(const glm::mat4& cameraViewProjectionMatrix);
private:
	void init() override;
	void update() override;
	void shutdown() override;

	GLRenderingManager m_renderingManager;
};

