#pragma once
#include "interface/IAssetManager.h"
#include "interface/ILogManager.h"

#include "entity/BaseGraphicPrimitive.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class assimpMeshRawData : public IMeshRawData
{
public:
	assimpMeshRawData();
	~assimpMeshRawData();

	int getNumVertices() const override;
	int getNumFaces() const override;
	int getNumIndicesInFace(int faceIndex) const override;
	vec3 getVertices(unsigned int index) const override;
	vec2 getTextureCoords(unsigned int index) const override;
	vec3 getNormals(unsigned int index) const override;
	int getIndices(int faceIndex, int index) const override;
	aiMesh* m_aiMesh;
};

class AssetManager : public IAssetManager
{
public:
	AssetManager() {};
	~AssetManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, BaseTexture* baseDexture) const override;
	modelPointerMap loadModelFromDisk(const std::string & fileName) const override;
	void parseloadRawModelData(const modelPointerMap & modelPointerMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, std::vector<BaseMesh*>& baseMesh, std::vector<BaseTexture*>& baseTexture)const override;
	std::string loadShader(const std::string& fileName) const override;

	const objectStatus& getStatus() const override;

protected:
	void setStatus(objectStatus objectStatus) override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	modelPointerMap processAssimpScene(const std::string& fileName, const aiScene* aiScene) const;
	modelPointerMap processAssimpNode(const std::string& fileName, aiNode* node, const aiScene* scene) const;
	textureFileNameMap processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial) const;

	std::unordered_map<std::string, int> m_supportedTextureType = { std::pair<std::string, int>("png", 0) };
	std::unordered_map<std::string, int> m_supportedModelType = { std::pair<std::string, int>("obj",0), std::pair<std::string, int>("innoModel", 0) };
	std::unordered_map<std::string, int> m_supportedShaderType = { std::pair<std::string, int>("sf", 0) };
	
	const std::string m_textureRelativePath = "../res/textures/";
	const std::string m_modelRelativePath = "../res/models/";
	const std::string m_shaderRelativePath = "../res/shaders/";
};

