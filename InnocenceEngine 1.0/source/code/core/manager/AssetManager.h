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

enum class unitMeshType { QUAD, CUBE, SPHERE };
class AssetManager : public IEventManager
{
public:
	~AssetManager();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static AssetManager& getInstance()
	{
		static AssetManager instance;
		return instance;
	}

	void loadAsset(const std::string& filePath, VisibleComponent& visibleComponent);
	void loadAsset(const std::string& filePath, textureType textureType, VisibleComponent& visibleComponent);

	std::string loadShader(const std::string& FileName) const;

	void loadCubeMapTextures(const std::vector<std::string>&  fileName, VisibleComponent& visibleComponent) const;

	void addUnitMesh(VisibleComponent& visibleComponent, unitMeshType unitMeshType);

private:
	AssetManager();

	enum class textureAssignType { ADD_DEFAULT, OVERWRITE };

	void loadShaderImpl(const std::string& filePath, std::string& fileContent);
	void loadModelImpl(const std::string& fileName, VisibleComponent& visibleComponent);
	void loadTextureImpl(const std::string& fileName, textureType textureType, VisibleComponent& visibleComponent);

	void assignloadedModel(graphicDataMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);

	graphicDataMap processAssimpScene(const std::string& fileName, const aiScene* aiScene, meshDrawMethod& meshDrawMethod, textureWrapMethod& textureWrapMethod);
	graphicDataMap processAssimpNode(const std::string& fileName, aiNode* node, const aiScene* scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod);
	meshDataID processSingleAssimpMesh(aiMesh* mesh, meshDrawMethod meshDrawMethod) const;
	void addVertexData(aiMesh * aiMesh, int vertexIndex, MeshData& meshData) const;
	textureDataMap processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod);

	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void assignLoadedTexture(textureAssignType textureAssignType, textureDataPair& loadedTextureDataPair, VisibleComponent& visibleComponent);

	textureDataID loadTextureFromDisk(const std::string & fileName, textureType textureType, textureWrapMethod textureWrapMethod);
	std::unordered_map<std::string, graphicDataMap> m_loadedModelMap;
	std::unordered_map<std::string, textureDataPair> m_loadedTextureMap;
	std::unordered_map<std::string, int> m_supportedTextureType = { std::pair<std::string, int>("png", 0) };
	std::unordered_map<std::string, int> m_supportedModelType = { std::pair<std::string, int>("obj",0), std::pair<std::string, int>("innoModel", 0) };
	const std::string m_textureRelativePath = "../res/textures/";
	const std::string m_modelRelativePath = "../res/models/";

	meshDataID m_UnitCubeTemplate;
	meshDataID m_UnitSphereTemplate;
	meshDataID m_UnitQuadTemplate;
	textureDataID m_basicNormalTemplate;
	textureDataID m_basicAlbedoTemplate;
	textureDataID m_basicMetallicTemplate;
	textureDataID m_basicRoughnessTemplate;
	textureDataID m_basicAOTemplate;
};

