#pragma once
#include "../interface/IVisibleGameEntity.h"
#include "../manager/IEventManager.h"
#include "../manager/LogManager.h"
#include "../platform-dependency/GL/GLRenderingManager.h"


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

	void getCameraTranslationMatrix(glm::mat4& t) const;
	void setCameraTranslationMatrix(const glm::mat4& t) ;
	void getCameraViewMatrix(glm::mat4& v) const;
	void setCameraViewMatrix(const glm::mat4& v);
	void getCameraProjectionMatrix(glm::mat4& p) const;
	void setCameraProjectionMatrix(const glm::mat4& p);

private:
	RenderingManager();

	void init() override;
	void update() override;
	void shutdown() override;
};

