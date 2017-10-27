#pragma once

#include "../../interface/IEventManager.h"
#include "../../manager/LogManager.h"
#include "SceneGraphManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "../../platform-dependency/GL/GLRenderingManager.h"
#include "GLGUIManager.h"
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

	void getCameraTranslationMatrix(glm::mat4& t) const;
	void setCameraPosMatrix(const glm::mat4& t) ;
	void getCameraViewMatrix(glm::mat4& v) const;
	void setCameraRotMatrix(const glm::mat4& v);
	void getCameraProjectionMatrix(glm::mat4& p) const;
	void setCameraProjectionMatrix(const glm::mat4& p);
	void getCameraPos(glm::vec3& pos) const;
	void setCameraPos(const glm::vec3& pos);

	void changeDrawPolygonMode() const;
	void toggleDepthBufferVisualizer();

private:
	RenderingManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	std::vector<std::unique_ptr<IEventManager>> m_childEventManager;
};

