#pragma once
#include "../interface/IEventManager.h"
#include "../manager/LogManager.h"
#include "../manager/graphic/RenderingManager.h"
#include "../data/GraphicData.h"
#include "../component/VisibleComponent.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class AssetManager : public IEventManager
{
public:
	~AssetManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	static AssetManager& getInstance()
	{
		static AssetManager instance;
		return instance;
	}

	std::string loadShader(const std::string& FileName) const;
	void importModel(const std::string& fileName) const;
	void loadModel(const std::string& fileName, VisibleComponent& visibleComponent);
	void loadTexture(const std::string& fileName, textureType textureType, VisibleComponent& visibleComponent) const;
	void loadTexture(const std::vector<std::string>&  fileName, VisibleComponent& visibleComponent) const;

	void addUnitCube(VisibleComponent& visibleComponent) const;
	void addUnitSphere(VisibleComponent& visibleComponent) const;
	void addUnitSkybox(VisibleComponent& visibleComponent) const;
	void addUnitQuad(VisibleComponent& visibleComponent) const;

private:
	AssetManager();

	void loadModelImpl(const std::string& fileName, VisibleComponent& visibleComponent);
	void processAssimpNode(const std::string& fileName, aiNode* node, const aiScene* scene, VisibleComponent & visibleComponent) const;
	void processAssimpMesh(aiMesh* mesh, VisibleComponent & visibleComponent) const;
	void processAssimpMaterial(const std::string& fileName, aiMaterial* material, VisibleComponent & visibleComponent) const;
	void loadTexture(const std::string& fileName, aiMaterial* material, aiTextureType aiTextureType, VisibleComponent & visibleComponent) const;
};

