#pragma once

#include "manager/BaseManager.h"
#include "manager/LogManager.h"
#include "SceneGraphManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "GLRenderingManager.h"

class RenderingManager : public BaseManager
{
public:
	RenderingManager() {};
	~RenderingManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void render();
	void shutdown() override;

	meshID addMesh();
	textureID add2DTexture();
	textureID add2DHDRTexture();
	textureID add3DTexture();
	textureID add3DHDRTexture();
	BaseMesh* getMesh(meshID meshID);
	Base2DTexture* get2DTexture(textureID textureID);
	Base2DTexture* get2DHDRTexture(textureID textureID);
	Base3DTexture* get3DTexture(textureID textureID);
	Base3DTexture* get3DHDRTexture(textureID textureID);

private:
	void changeDrawPolygonMode();
	void changeDrawTextureMode();

	std::vector<std::unique_ptr<IManager>> m_childManager;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;	
};

