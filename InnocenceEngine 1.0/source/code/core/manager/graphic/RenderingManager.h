#pragma once

#include "../../interface/IEventManager.h"
#include "../../manager/LogManager.h"
#include "SceneGraphManager.h"
#include "../../platform-dependency/GL/GLRenderingManager.h"
#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "../../data/GraphicData.h"

class RenderingManager : public IEventManager
{
public:
	~RenderingManager();

	static RenderingManager& getInstance()
	{
		static RenderingManager instance;
		return instance;
	}

	GLWindowManager& getWindowManager() const;
	GLInputManager& getInputManager() const;

	void render();

	void getCameraTranslationMatrix(glm::mat4& t) const;
	void setCameraTranslationMatrix(const glm::mat4& t) ;
	void getCameraViewMatrix(glm::mat4& v) const;
	void setCameraViewMatrix(const glm::mat4& v);
	void getCameraProjectionMatrix(glm::mat4& p) const;
	void setCameraProjectionMatrix(const glm::mat4& p);

private:
	RenderingManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	std::vector<std::unique_ptr<IEventManager>> m_childEventManager;
};

