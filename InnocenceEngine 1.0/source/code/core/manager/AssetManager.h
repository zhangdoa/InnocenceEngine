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
	~AssetManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static AssetManager& getInstance()
	{
		static AssetManager instance;
		return instance;
	}

	void loadAsset(const std::string& filePath);
	void loadAsset(const std::string& filePath, VisibleComponent& visibleComponent);
	void loadAsset(const std::string& filePath, textureType textureType, VisibleComponent& visibleComponent);

	std::string loadShader(const std::string& fileName) const;

	void loadCubeMapTextures(const std::vector<std::string>&  fileName, VisibleComponent& visibleComponent) const;

	void addUnitMesh(VisibleComponent& visibleComponent, unitMeshType unitMeshType);

private:
	AssetManager() {};

	enum class textureAssignType { ADD_DEFAULT, OVERWRITE };

	void loadShaderImpl(const std::string& filePath, std::string& fileContent);
	void loadModelImpl(const std::string& fileName, VisibleComponent& visibleComponent);
	void loadTextureImpl(const std::string& fileName, textureType textureType, VisibleComponent& visibleComponent);
	void loadModelImpl(const std::string& filePath);
	void loadTextureImpl(const std::string& filePath);
	void loadTextureFromDisk(const std::string& filePath);
	void loadShaderImpl(const std::string& filePath);

	void assignloadedModel(modelMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);

	modelMap processAssimpScene(const std::string& fileName, const aiScene* aiScene, meshDrawMethod& meshDrawMethod, textureWrapMethod& textureWrapMethod);
	modelMap processAssimpNode(const std::string& fileName, aiNode* node, const aiScene* scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod);
	meshID processSingleAssimpMesh(aiMesh* mesh, meshDrawMethod meshDrawMethod) const;
	void addVertexData(aiMesh * aiMesh, int vertexIndex, MeshData& meshData) const;
	textureMap processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod);

	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedTextureDataPair, VisibleComponent& visibleComponent);

	textureID loadTextureFromDisk(const std::string & fileName, textureType textureType, textureWrapMethod textureWrapMethod);

	std::unordered_map<std::string, modelMap> m_loadedModelMap;
	std::unordered_map<std::string, texturePair> m_loadedTextureMap;
	std::unordered_map<std::string, int> m_supportedTextureType = { std::pair<std::string, int>("png", 0) };
	std::unordered_map<std::string, int> m_supportedModelType = { std::pair<std::string, int>("obj",0), std::pair<std::string, int>("innoModel", 0) };
	std::unordered_map<std::string, int> m_supportedShaderType = { std::pair<std::string, int>("sf", 0) };
	const std::string m_textureRelativePath = "../res/textures/";
	const std::string m_modelRelativePath = "../res/models/";
	const std::string m_shaderRelativePath = "../res/shaders/";

	std::unordered_map<std::string, void*> m_rawTextureDatas;
	std::unordered_map<std::string, std::string> m_rawShaderDatas;

	meshID m_UnitCubeTemplate;
	meshID m_UnitSphereTemplate;
	meshID m_UnitQuadTemplate;
	textureID m_basicNormalTemplate;
	textureID m_basicAlbedoTemplate;
	textureID m_basicMetallicTemplate;
	textureID m_basicRoughnessTemplate;
	textureID m_basicAOTemplate;
};

