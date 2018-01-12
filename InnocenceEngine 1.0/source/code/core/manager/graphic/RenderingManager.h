#pragma once

#include "../../interface/IEventManager.h"
#include "../../manager/LogManager.h"
#include "SceneGraphManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "../../platform-dependency/GL/GLRenderingManager.h"
//#include "GLGUIManager.h"

class RenderingManager : public IEventManager
{
public:
	~RenderingManager();

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
	std::unordered_map<meshID, MeshData>& getMeshData();
	std::unordered_map<textureID, TextureData>& getTextureData();
	MeshData& getMeshData(meshID meshDataID);
	TextureData& getTextureData(textureID textureDataID);

private:
	RenderingManager();

	void AsyncRender();
	void changeDrawPolygonMode();
	void changeDrawTextureMode();

	std::vector<std::unique_ptr<IEventManager>> m_childEventManager;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;	
	std::unordered_map<meshID, MeshData> m_meshDatas;
	std::unordered_map<textureID, TextureData> m_textureDatas;
};

