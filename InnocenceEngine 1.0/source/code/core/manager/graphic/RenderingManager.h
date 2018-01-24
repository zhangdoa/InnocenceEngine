#pragma once

#include "../../interface/IManager.h"
#include "../../manager/LogManager.h"
#include "SceneGraphManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "../../platform-dependency/GL/GLRenderingManager.h"
//#include "GLGUIManager.h"

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

	meshID addMeshData();
	textureID addTextureData();
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
	std::unordered_map<meshID, IMesh*> m_meshMap;
	std::unordered_map<textureID, ITexture*> m_textureMap;
};

