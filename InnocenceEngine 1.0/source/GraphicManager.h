#pragma once
#include "IVisibleGameEntity.h"
#include "IEventManager.h"
#include "LogManager.h"
#include "GLRenderingManager.h"
#include "CameraComponent.h"


class GraphicManager : public IEventManager
{
public:
	~GraphicManager();

	static GraphicManager& getInstance()
	{
		static GraphicManager instance;
		return instance;
	}

	void render(IVisibleGameEntity* visibleGameEntity) const;
	void finishRender() const;
	void setCamera(CameraComponent* cameraComponent);

private:
	GraphicManager();

	void init() override;
	void update() override;
	void shutdown() override;
};

