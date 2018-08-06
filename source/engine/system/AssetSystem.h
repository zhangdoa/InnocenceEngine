#pragma once
#include <fstream>
#include <sstream>
#include <iostream>

#include "../common/config.h"
#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include <experimental/filesystem>
#endif

#include "common/UltiHeaders.h"
#include "interface/IAssetSystem.h"
#include "interface/ILogSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IGameSystem.h"
#include "MeshDataSystem.h"
#include "TextureDataSystem.h"
#include "common/ComponentHeaders.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

extern ILogSystem* g_pLogSystem;
extern IMemorySystem* g_pMemorySystem;
extern IGameSystem* g_pGameSystem;

class IMeshRawData
{
public:
	IMeshRawData() {};
	virtual ~IMeshRawData() {};

	virtual int getNumVertices() const = 0;
	virtual int getNumFaces() const = 0;
	virtual int getNumIndicesInFace(int faceIndex) const = 0;
	virtual vec4 getVertices(unsigned int index) const = 0;
	virtual vec2 getTextureCoords(unsigned int index) const = 0;
	virtual vec4 getNormals(unsigned int index) const = 0;
	virtual int getIndices(int faceIndex, int index) const = 0;
};

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

	meshID addMesh() override;
	meshID addMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) override;
	textureID addTexture(textureType textureType) override;
	MeshDataComponent* getMesh(meshID meshID) override;
	TextureDataComponent* getTexture(textureID textureID) override;
	MeshDataComponent* getDefaultMesh(meshShapeType meshShapeType) override;
	TextureDataComponent* getDefaultTexture(textureType textureType) override;
	void removeMesh(meshID meshID) override;
	void removeTexture(textureID textureID) override;
	vec4 findMaxVertex(meshID meshID) override;
	vec4 findMinVertex(meshID meshID) override;
	std::string loadShader(const std::string& fileName) const override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	MeshDataSystem* m_meshDataSystem;
	TextureDataSystem* m_textureDataSystem;
	AssetSystemSingletonComponent* m_AssetSystemSingletonComponent;

	void loadDefaultAssets();
	void loadAssetsForComponents();

	void loadTexture(const std::vector<std::string>& fileName, textureType textureType, VisibleComponent& visibleComponent);
	void loadModel(const std::string& fileName, VisibleComponent& visibleComponent);
	void loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, TextureDataComponent* baseDexture);
	void loadModelFromDisk(const std::string & fileName, modelMap& modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal);

	void processAssimpScene(const std::string& fileName, modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, const aiScene* aiScene, bool caclNormal);
	void processAssimpNode(const std::string& fileName, modelMap & modelMap, aiNode * node, const aiScene * scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal);
	void processSingleAssimpMesh(const std::string& fileName, meshID& meshID, aiMesh * aiMesh, meshDrawMethod meshDrawMethod, bool caclNormal);
	void processSingleAssimpMaterial(const std::string& fileName, textureMap & textureMap, const aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod);

	void assignUnitMesh(meshShapeType meshType, VisibleComponent& visibleComponent);
	void assignLoadedTexture(textureAssignType textureAssignType, const texturePair& loadedTextureDataPair, VisibleComponent& visibleComponent);
	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void assignLoadedModel(modelMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);
};

