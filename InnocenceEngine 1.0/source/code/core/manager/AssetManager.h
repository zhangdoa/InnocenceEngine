#pragma once
#include "IEventManager.h"
#include "../manager/LogManager.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class AssetManager : public IEventManager
{
public:
	~AssetManager();

	static AssetManager& getInstance()
	{
		static AssetManager instance;
		return instance;
	}

	std::string loadShader(const std::string& shaderFileName) const;
	void loadModel(const std::string& fileName) const;
private:
	AssetManager();

	void init() override;
	void update() override;
	void shutdown() override;

	void processAssimpNode(aiNode* node, const aiScene* scene, std::ofstream& file) const;
	void processAssimpMesh(aiMesh* mesh, const aiScene* scene, std::ofstream& file) const;

};

