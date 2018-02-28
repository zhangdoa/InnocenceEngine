#pragma once
#include "BaseManager.h"
#include "LogManager.h"

#include "entity/BaseGraphicPrimitive.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class AssetManager : public BaseManager
{
public:
	AssetManager() {};
	~AssetManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	textureID loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod) const;
	modelMap loadModelFromDisk(const std::string & fileName, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod);

	std::string loadShader(const std::string& fileName) const;

	static LogManager* g_pLogManager;

private:
	modelMap processAssimpScene(const std::string& fileName, const aiScene* aiScene, meshDrawMethod& meshDrawMethod, textureWrapMethod& textureWrapMethod);
	modelMap processAssimpNode(const std::string& fileName, aiNode* node, const aiScene* scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod);
	meshID processSingleAssimpMesh(aiMesh* mesh, meshDrawMethod meshDrawMethod) const;
	void addVertex(aiMesh * aiMesh, int vertexIndex, BaseMesh* mesh) const;
	textureMap processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod);

	std::unordered_map<std::string, int> m_supportedTextureType = { std::pair<std::string, int>("png", 0) };
	std::unordered_map<std::string, int> m_supportedModelType = { std::pair<std::string, int>("obj",0), std::pair<std::string, int>("innoModel", 0) };
	std::unordered_map<std::string, int> m_supportedShaderType = { std::pair<std::string, int>("sf", 0) };
	const std::string m_textureRelativePath = "../res/textures/";
	const std::string m_modelRelativePath = "../res/models/";
	const std::string m_shaderRelativePath = "../res/shaders/";
};

