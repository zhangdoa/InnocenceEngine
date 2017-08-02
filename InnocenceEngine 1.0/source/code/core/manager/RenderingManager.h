#pragma once
#include "../interface/IVisibleGameEntity.h"
#include "../manager/IEventManager.h"
#include "../manager/LogManager.h"
#include "../platform-dependency/GL/GLRenderingManager.h"
#include "../component/CameraComponent.h"


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

