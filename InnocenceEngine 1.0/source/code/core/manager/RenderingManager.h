#pragma once
#include "IVisibleGameEntity.h"
#include "IEventManager.h"
#include "LogManager.h"
#include "GLRenderingManager.h"
#include "CameraComponent.h"


class RenderingManager : public IEventManager
{
public:
	~RenderingManager();

	static RenderingManager& getInstance()
	{
		static RenderingManager instance;
		return instance;
	}

	void render(IVisibleGameEntity* visibleGameEntity) const;
	void finishRender() const;
	void setCamera(CameraComponent* cameraComponent);

private:
	RenderingManager();

	void init() override;
	void update() override;
	void shutdown() override;
};

