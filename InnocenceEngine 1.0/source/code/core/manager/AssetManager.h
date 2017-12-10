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
	void loadSingleTexture(const std::string& fileName, textureType textureType, VisibleComponent& visibleComponent);
	void loadCubeMapTextures(const std::vector<std::string>&  fileName, VisibleComponent& visibleComponent) const;

	void addUnitCube(VisibleComponent& visibleComponent);
	void addUnitSphere(VisibleComponent& visibleComponent);
	void addUnitQuad(VisibleComponent& visibleComponent);

private:
	AssetManager();

	void loadModelImpl(const std::string& fileName, VisibleComponent& visibleComponent);
	void assignloadedModel(graphicDataMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);
	void loadModelFromDisk(const std::string& fileName, VisibleComponent& visibleComponent);

	void processAssimpNode(const std::string& fileName, aiNode* node, const aiScene* scene, graphicDataMap& graphicDataMap, VisibleComponent & visibleComponent);
	void processSingleAssimpMesh(aiMesh* mesh, meshDataID meshDataID, meshDrawMethod meshDrawMethod) const;
	void addVertexData(aiMesh * aiMesh, int vertexIndex, MeshData * meshData) const;
	void processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod, textureDataMap& textureDataMap) const;
	void loadTextureForModel(const std::string& fileName, aiMaterial * aiMaterial, aiTextureType aiTextureType, textureWrapMethod textureWrapMethod, textureDataMap& textureDataMap) const;
	
	void assignDefaultTextures(VisibleComponent & visibleComponent);
	void assignloadedTexture(textureDataPair& loadedTextureDataPair, VisibleComponent& visibleComponent);

	textureDataID loadTextureFromDisk(const std::string & fileName, textureType textureType, textureWrapMethod textureWrapMethod);

	std::unordered_map<std::string, graphicDataMap> m_loadedModelMap;
	std::unordered_map<std::string, textureDataPair> m_loadedTextureMap;
	meshDataID m_UnitCubeTemplate;
	meshDataID m_UnitSphereTemplate;
	meshDataID m_UnitQuadTemplate;
	textureDataID m_basicNormalTemplate;
	textureDataID m_basicAlbedoTemplate;
	textureDataID m_basicMetallicTemplate;
	textureDataID m_basicRoughnessTemplate;
	textureDataID m_basicAOTemplate;
};

