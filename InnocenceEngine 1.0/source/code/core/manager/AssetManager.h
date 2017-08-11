#pragma once
#include "IEventManager.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "../data/GraphicData.h"

class AssetManager : public IEventManager
{
public:
	~AssetManager();

	static AssetManager& getInstance()
	{
		static AssetManager instance;
		return instance;
	}

	static void loadMesh(const std::string& filePath, const std::vector<StaticMeshData>& meshData);
	static void loadTexture(const std::string& filePath, const std::vector<TextureData>& textureData);
private:
	AssetManager();

	void init() override;
	void update() override;
	void shutdown() override;

	void processAssimpMesh(aiMesh *mesh, const aiScene* scene, const StaticMeshData* meshData);

};

