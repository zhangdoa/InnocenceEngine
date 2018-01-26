#pragma once

#include "../../interface/IManager.h"
#include "../../manager/LogManager.h"
#include "SceneGraphManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "../../platform-dependency/GL/GLRenderingManager.h"

class RenderingManager : public IManager
{
public:
	~RenderingManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static RenderingManager& getInstance()
	{
		static RenderingManager instance;
		return instance;
	}

	meshID addMesh();
	textureID addTexture();
	IMesh* getMesh(meshID meshID);
	ITexture* getTexture(textureID textureID);

private:
	RenderingManager() {};

	void AsyncRender();
	void changeDrawPolygonMode();
	void changeDrawTextureMode();

	std::vector<std::unique_ptr<IManager>> m_childManager;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;	
};

