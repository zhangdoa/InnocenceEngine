#pragma once
#include <fstream>
#include <sstream>
#include <iostream>

#include "common/UltiHeaders.h"
#include "interface/IAssetSystem.h"
#include "interface/ILogSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IGameSystem.h"
#include "MeshDataSystem.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "component/AssetSystemSingletonComponent.h"

extern ILogSystem* g_pLogSystem;
extern IMemorySystem* g_pMemorySystem;
extern IGameSystem* g_pGameSystem;

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

	meshID addMesh(meshType meshType) override;
	textureID addTexture(textureType textureType) override;
	MeshDataComponent* getMesh(meshType meshType, meshID meshID) override;
	TextureDataComponent* getTexture(textureType textureType, textureID textureID) override;
	void removeMesh(meshType meshType, meshID meshID) override;
	void removeTexture(textureID textureID) override;

	std::string loadShader(const std::string& fileName) const override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	MeshDataSystem* m_meshDataSystem;

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
	meshID addMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
};

