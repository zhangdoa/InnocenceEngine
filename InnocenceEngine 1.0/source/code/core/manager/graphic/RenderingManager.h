#pragma once

#include "../../interface/IEventManager.h"
#include "../../manager/LogManager.h"
#include "SceneGraphManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "../../platform-dependency/GL/GLRenderingManager.h"
//#include "GLGUIManager.h"
#include "../../data/GraphicData.h"

class RenderingManager : public IEventManager
{
public:
	~RenderingManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	static RenderingManager& getInstance()
	{
		static RenderingManager instance;
		return instance;
	}

	void initInput();
	void changeDrawPolygonMode() const;
	void toggleDepthBufferVisualizer();
	GameObjectID addMeshData();
	GameObjectID addTextureData();
	std::unordered_map<GameObjectID, MeshData>& getMeshData();
	std::unordered_map<GameObjectID, TextureData>& getTextureData();
	MeshData& getMeshData(GameObjectID meshDataIndex);
	TextureData& getTextureData(GameObjectID textureDataIndex);

private:
	RenderingManager();

	std::thread* m_asyncRenderThread;
	void AsyncRender();
	std::vector<std::unique_ptr<IEventManager>> m_childEventManager;

	std::unordered_map<GameObjectID, MeshData> m_meshDatas;
	std::unordered_map<GameObjectID, TextureData> m_textureDatas;
};

