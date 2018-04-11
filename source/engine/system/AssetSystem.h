#pragma once
#include <fstream>
#include <sstream>
#include <iostream>

#include "common/UltiHeaders.h"
#include "interface/IAssetSystem.h"
#include "interface/ILogSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IRenderingSystem.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

extern ILogSystem* g_pLogSystem;
extern IMemorySystem* g_pMemorySystem;
extern IRenderingSystem* g_pRenderingSystem;

class assimpMeshRawData : public IMeshRawData
{
public:
	assimpMeshRawData() {};
	~assimpMeshRawData() {};

	int getNumVertices() const override;
	int getNumFaces() const override;
	int getNumIndicesInFace(int faceIndex) const override;
	vec4 getVertices(unsigned int index) const override;
	vec2 getTextureCoords(unsigned int index) const override;
	vec4 getNormals(unsigned int index) const override;
	int getIndices(int faceIndex, int index) const override;
	aiMesh* m_aiMesh;
};

class AssetSystem : public IAssetSystem
{
public:
	AssetSystem() {};
	~AssetSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, BaseTexture* baseDexture) const override;
	void loadModelFromDisk(const std::string & fileName, modelMap& modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal) override;
	std::string loadShader(const std::string& fileName) const override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void processAssimpScene(const std::string& fileName, modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, const aiScene* aiScene, bool caclNormal);
	void processAssimpNode(const std::string& fileName, modelMap & modelMap, aiNode * node, const aiScene * scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal);
	void processSingleAssimpMesh(const std::string& fileName, meshID& meshID, aiMesh * aiMesh, meshDrawMethod meshDrawMethod, bool caclNormal) const;
	void processSingleAssimpMaterial(const std::string& fileName, textureMap & textureMap, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod);

	std::unordered_map<std::string, int> m_supportedTextureType = { std::pair<std::string, int>("png", 0) };
	std::unordered_map<std::string, int> m_supportedModelType = { std::pair<std::string, int>("obj",0), std::pair<std::string, int>("innoModel", 0) };
	std::unordered_map<std::string, int> m_supportedShaderType = { std::pair<std::string, int>("sf", 0) };
	
	std::unordered_map<std::string, texturePair> m_loadedTexture;
	const std::string m_textureRelativePath = "../res/textures/";
	const std::string m_modelRelativePath = "../res/models/";
	const std::string m_shaderRelativePath = "../res/shaders/";
};

