#include "AssetSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "STB_Image/stb_image.h"
#include "../LowLevelSystem/MemorySystem.h"
#include "../LowLevelSystem/LogSystem.h"
#include "../LowLevelSystem/TaskSystem.h"
#include "../../component/AssetSystemSingletonComponent.h"
#include "../../component/GameSystemSingletonComponent.h"
#include "MeshDataSystem.h"
#include "TextureDataSystem.h"

namespace InnoAssetSystem
{
	void loadDefaultAssets();
	void loadAssetsForComponents();

	void loadModel(const std::string& fileName, VisibleComponent& visibleComponent);
	texturePair loadTexture(const std::string &fileName, textureType textureType, textureWrapMethod textureWrapMethod);
	void loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, TextureDataComponent* baseDexture);
	void loadModelFromDisk(const std::string & fileName, modelMap& modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal);

	void processAssimpScene(modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, const aiScene* aiScene, bool caclNormal);
	void processAssimpNode(modelMap & modelMap, aiNode * node, const aiScene * scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal);
	void processSingleAssimpMesh(EntityID& EntityID, aiMesh * aiMesh, meshDrawMethod meshDrawMethod, bool caclNormal);
	void processSingleAssimpMaterial(textureMap & textureMap, const aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod);
	
	EntityID addMeshDataComponent();
	EntityID addMeshDataComponent(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
	EntityID addTextureDataComponent(textureType textureType);

	void assignUnitMesh(meshShapeType meshType, VisibleComponent& visibleComponent);
	void assignLoadedTexture(textureAssignType textureAssignType, const texturePair& loadedTextureDataPair, VisibleComponent& visibleComponent);
	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void assignLoadedModel(modelMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);
	void addMeshData(VisibleComponent* visibleComponentconst, EntityID & EntityID);
	void addTextureData(VisibleComponent* visibleComponentconst, const texturePair & texturePair);
	void overwriteTextureData(VisibleComponent* visibleComponentconst, const texturePair & texturePair);

	static AssetSystemSingletonComponent* g_AssetSystemSingletonComponent;
	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;

	objectStatus m_AssetSystemStatus = objectStatus::SHUTDOWN;
}

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

	int getNumVertices() const;
	int getNumFaces() const;
	int getNumIndicesInFace(int faceIndex) const;
	vec4 getVertices(unsigned int index) const;
	vec2 getTextureCoords(unsigned int index) const;
	vec4 getNormals(unsigned int index) const;
	int getIndices(int faceIndex, int index) const;
	aiMesh* m_aiMesh = 0;
};

InnoHighLevelSystem_EXPORT bool InnoAssetSystem::setup()
{
	g_AssetSystemSingletonComponent = &AssetSystemSingletonComponent::getInstance();
	g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();

	m_AssetSystemStatus = objectStatus::ALIVE;
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoAssetSystem::initialize()
{
	loadDefaultAssets();
	// @TODO: more granularly do IO operations
	g_AssetSystemSingletonComponent->m_asyncTaskVector.push_back(InnoTaskSystem::submit([]()
	{
		loadAssetsForComponents();
	}));

	InnoLogSystem::printLog("AssetSystem has been initialized.");
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoAssetSystem::update()
{
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoAssetSystem::terminate()
{
	m_AssetSystemStatus = objectStatus::STANDBY;
	m_AssetSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("AssetSystem has been terminated.");
	return true;
}

// @TODO: merge to a text file parser
std::string InnoAssetSystem::loadShader(const std::string & fileName)
{
	std::ifstream file;
	file.open((g_AssetSystemSingletonComponent->m_shaderRelativePath + fileName).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	output = shaderStream.str();
	file.close();

	return output;
}

objectStatus InnoAssetSystem::getStatus()
{
	return m_AssetSystemStatus;
}

EntityID InnoAssetSystem::addMeshDataComponent()
{
	MeshDataComponent* newMesh = InnoMemorySystem::spawn<MeshDataComponent>();
	auto l_parentEntity = InnoMath::createEntityID();
	newMesh->m_parentEntity = l_parentEntity;
	auto l_meshMap = &g_AssetSystemSingletonComponent->m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, MeshDataComponent*>(l_parentEntity, newMesh));
	return l_parentEntity;
}

EntityID InnoAssetSystem::addMeshDataComponent(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	auto l_MeshID = addMeshDataComponent();
	auto l_MeshData = getMeshDataComponent(l_MeshID);

	l_MeshData->m_vertices = std::move(vertices);
	l_MeshData->m_indices = std::move(indices);

	return l_MeshID;
}

EntityID InnoAssetSystem::addTextureDataComponent(textureType textureType)
{
	TextureDataComponent* newTexture = InnoMemorySystem::spawn<TextureDataComponent>();
	auto l_parentEntity = InnoMath::createEntityID();
	newTexture->m_parentEntity = l_parentEntity;
	auto l_textureMap = &g_AssetSystemSingletonComponent->m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, TextureDataComponent*>(l_parentEntity, newTexture));
	return l_parentEntity;
}

MeshDataComponent* InnoAssetSystem::getMeshDataComponent(EntityID EntityID)
{
	auto result = g_AssetSystemSingletonComponent->m_meshMap.find(EntityID);
	if (result != g_AssetSystemSingletonComponent->m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

TextureDataComponent * InnoAssetSystem::getTextureDataComponent(EntityID EntityID)
{
	auto result = g_AssetSystemSingletonComponent->m_textureMap.find(EntityID);
	if (result != g_AssetSystemSingletonComponent->m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

MeshDataComponent * InnoAssetSystem::getDefaultMeshDataComponent(meshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case meshShapeType::LINE:
		return getMeshDataComponent(g_AssetSystemSingletonComponent->m_UnitLineTemplate); break;
	case meshShapeType::QUAD:
		return getMeshDataComponent(g_AssetSystemSingletonComponent->m_UnitQuadTemplate); break;
	case meshShapeType::CUBE:
		return getMeshDataComponent(g_AssetSystemSingletonComponent->m_UnitCubeTemplate); break;
	case meshShapeType::SPHERE:
		return getMeshDataComponent(g_AssetSystemSingletonComponent->m_UnitSphereTemplate); break;
	case meshShapeType::CUSTOM:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoAssetSystem::getDefaultTextureDataComponent(textureType textureType)
{
	switch (textureType)
	{
	case textureType::INVISIBLE:
		return nullptr; break;
	case textureType::NORMAL:
		return getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicNormalTemplate); break;
	case textureType::ALBEDO:
		return getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicAlbedoTemplate); break;
	case textureType::METALLIC:
		return getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicMetallicTemplate); break;
	case textureType::ROUGHNESS:
		return getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicRoughnessTemplate); break;
	case textureType::AMBIENT_OCCLUSION:
		return getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicAOTemplate); break;
	case textureType::CUBEMAP:
		return nullptr; break;
	case textureType::ENVIRONMENT_CAPTURE:
		return nullptr; break;
	case textureType::ENVIRONMENT_CONVOLUTION:
		return nullptr; break;
	case textureType::ENVIRONMENT_PREFILTER:
		return nullptr; break;
	case textureType::EQUIRETANGULAR:
		return nullptr; break;
	case textureType::RENDER_BUFFER_SAMPLER:
		return nullptr; break;
	case textureType::SHADOWMAP:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

void InnoAssetSystem::removeMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &g_AssetSystemSingletonComponent->m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		InnoMemorySystem::destroy<MeshDataComponent>(l_mesh->second);
		l_meshMap->erase(EntityID);
	}
}

void InnoAssetSystem::removeTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &g_AssetSystemSingletonComponent->m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			stbi_image_free(i);
		}
		
		InnoMemorySystem::destroy<TextureDataComponent>(l_texture->second);
		l_textureMap->erase(EntityID);
	}
}

void InnoAssetSystem::releaseRawDataForMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &g_AssetSystemSingletonComponent->m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		l_mesh->second->m_vertices.clear();
		l_mesh->second->m_vertices.resize(0);
		l_mesh->second->m_vertices.shrink_to_fit();
		l_mesh->second->m_indices.clear();
		l_mesh->second->m_indices.resize(0);
		l_mesh->second->m_indices.shrink_to_fit();
	}
}

void InnoAssetSystem::releaseRawDataForTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &g_AssetSystemSingletonComponent->m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			stbi_image_free(i);
		}
	}
}

vec4 InnoAssetSystem::findMaxVertex(EntityID EntityID)
{
	return MeshDataSystem::findMaxVertex(*getMeshDataComponent(EntityID));
}

vec4 InnoAssetSystem::findMinVertex(EntityID EntityID)
{
	return MeshDataSystem::findMinVertex(*getMeshDataComponent(EntityID));
}

void InnoAssetSystem::loadDefaultAssets()
{
	g_AssetSystemSingletonComponent->m_basicNormalTemplate = addTextureDataComponent(textureType::NORMAL);
	g_AssetSystemSingletonComponent->m_basicAlbedoTemplate = addTextureDataComponent(textureType::ALBEDO);
	g_AssetSystemSingletonComponent->m_basicMetallicTemplate = addTextureDataComponent(textureType::METALLIC);
	g_AssetSystemSingletonComponent->m_basicRoughnessTemplate = addTextureDataComponent(textureType::ROUGHNESS);
	g_AssetSystemSingletonComponent->m_basicAOTemplate = addTextureDataComponent(textureType::AMBIENT_OCCLUSION);

	loadTextureFromDisk({ "basic_normal.png" }, textureType::NORMAL, textureWrapMethod::REPEAT, getTextureDataComponent( g_AssetSystemSingletonComponent->m_basicNormalTemplate));
	loadTextureFromDisk({ "basic_albedo.png" }, textureType::ALBEDO, textureWrapMethod::REPEAT, getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicAlbedoTemplate));
	loadTextureFromDisk({ "basic_metallic.png" }, textureType::METALLIC, textureWrapMethod::REPEAT, getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicMetallicTemplate));
	loadTextureFromDisk({ "basic_roughness.png" }, textureType::ROUGHNESS, textureWrapMethod::REPEAT, getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicRoughnessTemplate));
	loadTextureFromDisk({ "basic_ao.png" }, textureType::AMBIENT_OCCLUSION, textureWrapMethod::REPEAT, getTextureDataComponent(g_AssetSystemSingletonComponent->m_basicAOTemplate));

	g_AssetSystemSingletonComponent->m_UnitLineTemplate = addMeshDataComponent();
	auto lastLineMeshData = getMeshDataComponent(g_AssetSystemSingletonComponent->m_UnitLineTemplate);
	MeshDataSystem::addUnitLine(*lastLineMeshData);
	lastLineMeshData->m_meshType = meshType::NORMAL;
	lastLineMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	lastLineMeshData->m_calculateNormals = false;
	lastLineMeshData->m_calculateTangents = false;
	lastLineMeshData->m_objectStatus = objectStatus::STANDBY;
	g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(lastLineMeshData);

	g_AssetSystemSingletonComponent->m_UnitQuadTemplate = addMeshDataComponent();
	auto lastQuadMeshData = getMeshDataComponent(g_AssetSystemSingletonComponent->m_UnitQuadTemplate);
	MeshDataSystem::addUnitQuad(*lastQuadMeshData);
	lastQuadMeshData->m_meshType = meshType::NORMAL;
	lastQuadMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	lastQuadMeshData->m_calculateNormals = false;
	lastQuadMeshData->m_calculateTangents = false;
	lastQuadMeshData->m_objectStatus = objectStatus::STANDBY;
	g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(lastQuadMeshData);

	g_AssetSystemSingletonComponent->m_UnitCubeTemplate = addMeshDataComponent();
	auto lastCubeMeshData = getMeshDataComponent(g_AssetSystemSingletonComponent->m_UnitCubeTemplate);
	MeshDataSystem::addUnitCube(*lastCubeMeshData);
	lastCubeMeshData->m_meshType = meshType::NORMAL;
	lastCubeMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	lastCubeMeshData->m_calculateNormals = false;
	lastCubeMeshData->m_calculateTangents = false;
	lastCubeMeshData->m_objectStatus = objectStatus::STANDBY;
	g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(lastCubeMeshData);

	g_AssetSystemSingletonComponent->m_UnitSphereTemplate = addMeshDataComponent();
	auto lastSphereMeshData = getMeshDataComponent(g_AssetSystemSingletonComponent->m_UnitSphereTemplate);
	MeshDataSystem::addUnitSphere(*lastSphereMeshData);
	lastSphereMeshData->m_meshType = meshType::NORMAL;
	lastSphereMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	lastSphereMeshData->m_calculateNormals = false;
	lastSphereMeshData->m_calculateTangents = false;
	lastSphereMeshData->m_objectStatus = objectStatus::STANDBY;
	g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(lastSphereMeshData);
}

void InnoAssetSystem::loadAssetsForComponents()
{
	for (auto& l_cameraComponent : g_GameSystemSingletonComponent->m_cameraComponents)
	{
		if (l_cameraComponent->m_drawAABB)
		{
			auto l_EntityID = addMeshDataComponent(l_cameraComponent->m_AABB.m_vertices, l_cameraComponent->m_AABB.m_indices);
			auto l_Mesh = InnoAssetSystem::getMeshDataComponent(l_EntityID);
			l_Mesh->m_objectStatus = objectStatus::STANDBY;
			g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_Mesh);
			l_cameraComponent->m_AABBMeshID = l_EntityID;
		}
		if (l_cameraComponent->m_drawFrustum)
		{
			auto l_EntityID = addMeshDataComponent(l_cameraComponent->m_frustumVertices, l_cameraComponent->m_frustumIndices);
			auto l_Mesh = getMeshDataComponent(l_EntityID);
			l_Mesh->m_objectStatus = objectStatus::STANDBY;
			g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_Mesh);
			l_cameraComponent->m_FrustumMeshID = l_EntityID;
		}
	}
	for (auto& l_lightComponent : g_GameSystemSingletonComponent->m_lightComponents)
	{
		if (l_lightComponent->m_drawAABB)
		{
			auto l_containerSize = l_lightComponent->m_AABBs.size();
			l_lightComponent->m_AABBMeshIDs.reserve(l_containerSize);
			
			for (size_t i = 0; i < l_containerSize; i++)
			{
				auto l_EntityID = addMeshDataComponent(l_lightComponent->m_AABBs[i].m_vertices, l_lightComponent->m_AABBs[i].m_indices);
				auto l_Mesh = getMeshDataComponent(l_EntityID);
				l_Mesh->m_objectStatus = objectStatus::STANDBY;
				g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_Mesh);
				l_lightComponent->m_AABBMeshIDs.emplace_back(l_EntityID);
			}
		}
	}

	for (auto& l_visibleComponent : g_GameSystemSingletonComponent->m_visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType != visiblilityType::INVISIBLE)
		{
			if (l_visibleComponent->m_meshShapeType == meshShapeType::CUSTOM)
			{
				if (l_visibleComponent->m_modelFileName != "")
				{
					loadModel(l_visibleComponent->m_modelFileName, *l_visibleComponent);
				}
			}
			else
			{
				assignUnitMesh(l_visibleComponent->m_meshShapeType, *l_visibleComponent);
				assignDefaultTextures(textureAssignType::OVERWRITE, *l_visibleComponent);
			}
			if (l_visibleComponent->m_drawAABB)
			{
				auto l_EntityID = InnoAssetSystem::addMeshDataComponent(l_visibleComponent->m_AABB.m_vertices, l_visibleComponent->m_AABB.m_indices);
				auto l_Mesh = InnoAssetSystem::getMeshDataComponent(l_EntityID);
				l_Mesh->m_objectStatus = objectStatus::STANDBY;
				g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_Mesh);
				l_visibleComponent->m_AABBMeshID = l_EntityID;
			}
		}
	}
	for (auto& l_environmentCaptureComponent : g_GameSystemSingletonComponent->m_environmentCaptureComponents)
	{
		l_environmentCaptureComponent->m_texturePair = loadTexture(l_environmentCaptureComponent->m_cubemapTextureFileName, textureType::EQUIRETANGULAR, textureWrapMethod::CLAMP_TO_EDGE);
	}
}

void InnoAssetSystem::assignUnitMesh(meshShapeType meshType, VisibleComponent & visibleComponent)
{
    EntityID l_UnitMeshTemplate;
    switch (meshType)
    {
            case meshShapeType::LINE: l_UnitMeshTemplate = g_AssetSystemSingletonComponent->m_UnitLineTemplate; break;
            case meshShapeType::QUAD: l_UnitMeshTemplate = g_AssetSystemSingletonComponent->m_UnitQuadTemplate; break;
            case meshShapeType::CUBE: l_UnitMeshTemplate = g_AssetSystemSingletonComponent->m_UnitCubeTemplate; break;
            case meshShapeType::SPHERE: l_UnitMeshTemplate = g_AssetSystemSingletonComponent->m_UnitSphereTemplate; break;
            case meshShapeType::CUSTOM: break;
    }
	InnoAssetSystem::addMeshData(&visibleComponent, l_UnitMeshTemplate);
}

void InnoAssetSystem::assignLoadedTexture(textureAssignType textureAssignType, const texturePair& loadedtexturePair, VisibleComponent & visibleComponent)
{
	if (textureAssignType == textureAssignType::ADD)
	{
		InnoAssetSystem::addTextureData(&visibleComponent, loadedtexturePair);
	}
	else if (textureAssignType == textureAssignType::OVERWRITE)
	{
		InnoAssetSystem::overwriteTextureData(&visibleComponent, loadedtexturePair);
	}
}

void InnoAssetSystem::assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent)
{
	if (visibleComponent.m_visiblilityType == visiblilityType::STATIC_MESH)
	{
		assignLoadedTexture(textureAssignType, texturePair(textureType::NORMAL, g_AssetSystemSingletonComponent->m_basicNormalTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ALBEDO, g_AssetSystemSingletonComponent->m_basicAlbedoTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::METALLIC, g_AssetSystemSingletonComponent->m_basicMetallicTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ROUGHNESS, g_AssetSystemSingletonComponent->m_basicRoughnessTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::AMBIENT_OCCLUSION, g_AssetSystemSingletonComponent->m_basicAOTemplate), visibleComponent);
	}
}

void InnoAssetSystem::assignLoadedModel(modelMap& loadedmodelMap, VisibleComponent & visibleComponent)
{
	visibleComponent.m_modelMap = loadedmodelMap;
	assignDefaultTextures(textureAssignType::ADD, visibleComponent);
}

void InnoAssetSystem::addMeshData(VisibleComponent * visibleComponent, EntityID & EntityID)
{
	visibleComponent->m_modelMap.emplace(EntityID, textureMap());
}

void InnoAssetSystem::addTextureData(VisibleComponent * visibleComponent, const texturePair & texturePair)
{
	for (auto& l_model : visibleComponent->m_modelMap)
	{
		auto l_texturePair = l_model.second.find(texturePair.first);
		if (l_texturePair == l_model.second.end())
		{
			l_model.second.emplace(texturePair);
		}
	}
}

void InnoAssetSystem::overwriteTextureData(VisibleComponent * visibleComponent, const texturePair & texturePair)
{
	for (auto& l_model : visibleComponent->m_modelMap)
	{
		auto l_texturePair = l_model.second.find(texturePair.first);
		if (l_texturePair == l_model.second.end())
		{
			l_model.second.emplace(texturePair);
		}
		else
		{
			l_texturePair->second = texturePair.second;
		}
	}
}

texturePair InnoAssetSystem::loadTexture(const std::string &fileName, textureType textureType, textureWrapMethod textureWrapMethod)
{
	auto l_loadedTexturePair = g_AssetSystemSingletonComponent->m_loadedTextureMap.find(fileName);
	if (l_loadedTexturePair != g_AssetSystemSingletonComponent->m_loadedTextureMap.end())
	{
		InnoLogSystem::printLog("AssetSystem: innoTexture: " + fileName + " is already loaded.");
		return l_loadedTexturePair->second;
	}
	else
	{
		auto l_textureDataID = addTextureDataComponent(textureType);
		auto l_textureData = getTextureDataComponent(l_textureDataID);

		loadTextureFromDisk({ fileName }, textureType, textureWrapMethod, l_textureData);

		g_AssetSystemSingletonComponent->m_loadedTextureMap.emplace(fileName, texturePair(textureType, l_textureDataID));
		
		return texturePair(textureType, l_textureDataID);
	}

}

// @TODO: return result
void InnoAssetSystem::loadModel(const std::string & fileName, VisibleComponent & visibleComponent)
{
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	// check if this file has already been loaded once
	auto l_loadedmodelMap = g_AssetSystemSingletonComponent->m_loadedModelMap.find(l_convertedFilePath);
	if (l_loadedmodelMap != g_AssetSystemSingletonComponent->m_loadedModelMap.end())
	{
		assignLoadedModel(l_loadedmodelMap->second, visibleComponent);

		InnoLogSystem::printLog("AssetSystem: innoMesh: " + l_convertedFilePath + " is already loaded, successfully assigned loaded modelMap.");
	}
	else
	{
		modelMap l_modelMap;
		loadModelFromDisk(fileName, l_modelMap, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod, visibleComponent.m_caclNormal);
		assignLoadedModel(l_modelMap, visibleComponent);

		//mark as loaded
		g_AssetSystemSingletonComponent->m_loadedModelMap.emplace(l_convertedFilePath, l_modelMap);
	}
}

void InnoAssetSystem::loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, TextureDataComponent* baseTexture)
{
	if (textureType == textureType::CUBEMAP)
	{
		int width, height, nrChannels;

		auto l_containerSize = fileName.size();
		std::vector<void*> l_3DTextureRawData(l_containerSize);

		for (auto i = (unsigned int)0; i < l_containerSize; i++)
		{
			// load image, do not flip texture
			stbi_set_flip_vertically_on_load(false);
			auto *data = stbi_load((g_AssetSystemSingletonComponent->m_textureRelativePath + fileName[i]).c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				l_3DTextureRawData.emplace_back(data);
				InnoLogSystem::printLog("innoTexture: " + fileName[i] + " is loaded.");
			}
			else
			{
				InnoLogSystem::printLog("Error::STBI:: Failed to load texture: " + (g_AssetSystemSingletonComponent->m_textureRelativePath + fileName[i]));
				return;
			}
		}

		baseTexture->m_textureType = textureType::CUBEMAP;
		baseTexture->m_textureColorComponentsFormat = textureColorComponentsFormat::SRGB;
		baseTexture->m_texturePixelDataFormat = texturePixelDataFormat(nrChannels - 1);
		baseTexture->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
		baseTexture->m_textureMinFilterMethod = textureFilterMethod::LINEAR;
		baseTexture->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
		baseTexture->m_textureWidth = width;
		baseTexture->m_textureHeight = height;
		baseTexture->m_texturePixelDataType = texturePixelDataType::UNSIGNED_BYTE;
		baseTexture->m_textureData = std::move(l_3DTextureRawData);
		baseTexture->m_objectStatus = objectStatus::STANDBY;
		g_AssetSystemSingletonComponent->m_uninitializedTextureComponents.push(baseTexture);

		InnoLogSystem::printLog("AssetSystem: innoTexture: cubemap texture is fully loaded.");
	}
	else if (textureType == textureType::EQUIRETANGULAR)
	{
		int width, height, nrChannels;
		// load image, flip texture
		stbi_set_flip_vertically_on_load(true);
		auto *data = stbi_loadf((g_AssetSystemSingletonComponent->m_textureRelativePath + fileName[0]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			baseTexture->m_textureType = textureType::EQUIRETANGULAR;
			baseTexture->m_textureColorComponentsFormat = textureColorComponentsFormat::RGB16F;
			baseTexture->m_texturePixelDataFormat = texturePixelDataFormat(nrChannels - 1);
			baseTexture->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
			baseTexture->m_textureMinFilterMethod = textureFilterMethod::LINEAR;
			baseTexture->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
			baseTexture->m_textureWidth = width;
			baseTexture->m_textureHeight = height;
			baseTexture->m_texturePixelDataType = texturePixelDataType::FLOAT;
			baseTexture->m_textureData = { data };
			baseTexture->m_objectStatus = objectStatus::STANDBY;
			g_AssetSystemSingletonComponent->m_uninitializedTextureComponents.push(baseTexture);

			InnoLogSystem::printLog("AssetSystem: innoTexture: " + fileName[0] + " is loaded.");
		}
		else
		{
			InnoLogSystem::printLog("Error::STBI:: Failed to load texture: " + (g_AssetSystemSingletonComponent->m_textureRelativePath + fileName[0]));
			return;
		}
	}
	else
	{
		int width, height, nrChannels;
		// load image
		stbi_set_flip_vertically_on_load(true);

		auto *data = stbi_load((g_AssetSystemSingletonComponent->m_textureRelativePath + fileName[0]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			baseTexture->m_textureType = textureType;
			baseTexture->m_textureColorComponentsFormat = textureColorComponentsFormat(nrChannels - 1);
			baseTexture->m_texturePixelDataFormat = texturePixelDataFormat(nrChannels - 1);
			baseTexture->m_textureWrapMethod = textureWrapMethod;
			baseTexture->m_textureMinFilterMethod = textureFilterMethod::LINEAR_MIPMAP_LINEAR;
			baseTexture->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
			baseTexture->m_textureWidth = width;
			baseTexture->m_textureHeight = height;
			baseTexture->m_texturePixelDataType = texturePixelDataType::UNSIGNED_BYTE;
			baseTexture->m_textureData = { data };
			baseTexture->m_objectStatus = objectStatus::STANDBY;
			g_AssetSystemSingletonComponent->m_uninitializedTextureComponents.push(baseTexture);

			InnoLogSystem::printLog("AssetSystem: innoTexture: " + fileName[0] + " is loaded.");
		}
		else
		{
			InnoLogSystem::printLog("Error::STBI:: Failed to load texture: " + fileName[0]);
			return;
		}
	}
}

void InnoAssetSystem::loadModelFromDisk(const std::string & fileName, modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal)
{
	// read file via ASSIMP
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	Assimp::Importer l_assImporter;
	const aiScene* l_assScene;
#if defined INNO_PLATFORM_WIN32 || defined INNO_PLATFORM_WIN64
	// try to load .innoModel first
	if (std::experimental::filesystem::exists(std::experimental::filesystem::path(g_AssetSystemSingletonComponent->m_modelRelativePath + l_convertedFilePath)))
	{
		l_assScene = l_assImporter.ReadFile(g_AssetSystemSingletonComponent->m_modelRelativePath + l_convertedFilePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	}
	else if (std::experimental::filesystem::exists(std::experimental::filesystem::path(g_AssetSystemSingletonComponent->m_modelRelativePath + fileName)))
	{
		// try to load original file then
		l_assScene = l_assImporter.ReadFile(g_AssetSystemSingletonComponent->m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// save model file as .innoModel binary file
		Assimp::Exporter l_assExporter;
		l_assExporter.Export(l_assScene, "assbin", g_AssetSystemSingletonComponent->m_modelRelativePath + fileName.substr(0, fileName.find(".")) + ".innoModel", 0u, 0);
		InnoLogSystem::printLog("AssetSystem: " + fileName + " is successfully converted.");
	}
	else
	{
		InnoLogSystem::printLog("AssetSystem: " + fileName + " doesn't exist!");
		return;
	}
#else
	// try to load .innoModel first
    l_assScene = l_assImporter.ReadFile(g_AssetSystemSingletonComponent->m_modelRelativePath + l_convertedFilePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (l_assScene == nullptr)
    {
		// try to load original file then
        l_assScene = l_assImporter.ReadFile(g_AssetSystemSingletonComponent->m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		if (l_assScene == nullptr)
		{
			InnoLogSystem::printLog("AssetSystem: " + fileName + " doesn't exist!");
			return;
		}
		// save model file as .innoModel binary file
        Assimp::Exporter l_assExporter;
        l_assExporter.Export(l_assScene, "assbin", g_AssetSystemSingletonComponent->m_modelRelativePath + fileName.substr(0, fileName.find(".")) + ".innoModel", 0u, 0);
        InnoLogSystem::printLog("AssetSystem: " + fileName + " is successfully converted.");
    }
#endif
	if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		InnoLogSystem::printLog("Error:ASSIMP: " + std::string{ l_assImporter.GetErrorString() });
		return;
	}

	processAssimpScene(modelMap, meshDrawMethod, textureWrapMethod, l_assScene, caclNormal);

	InnoLogSystem::printLog("AssetSystem: " + fileName + " is loaded for the first time, successfully assigned modelMap IDs.");
}

void InnoAssetSystem::processAssimpScene(modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, const aiScene* aiScene, bool caclNormal)
{
	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		processAssimpNode(modelMap, aiScene->mRootNode, aiScene, meshDrawMethod, textureWrapMethod, caclNormal);
	}
	for (auto i = (unsigned int)0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			processAssimpNode(modelMap, aiScene->mRootNode->mChildren[i], aiScene, meshDrawMethod, textureWrapMethod, caclNormal);
		}
	}
}

void InnoAssetSystem::processAssimpNode(modelMap & modelMap, aiNode * node, const aiScene * scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal)
{
	// process each mesh located at the current node
	for (auto i = (unsigned int)0; i < node->mNumMeshes; i++)
	{
		auto l_modelPair = modelPair();
		auto l_aiMesh = scene->mMeshes[node->mMeshes[i]];
		processSingleAssimpMesh(l_modelPair.first, l_aiMesh, meshDrawMethod, caclNormal);

		// process material
		if (l_aiMesh->mMaterialIndex > 0)
		{
			processSingleAssimpMaterial(l_modelPair.second, scene->mMaterials[l_aiMesh->mMaterialIndex], textureWrapMethod);
		}
		modelMap.emplace(l_modelPair);
	}
}

void InnoAssetSystem::processSingleAssimpMesh(EntityID& EntityID, aiMesh * aiMesh, meshDrawMethod meshDrawMethod, bool caclNormal)
{
	EntityID = addMeshDataComponent();
	auto l_meshData = getMeshDataComponent(EntityID);
	auto l_containerSize = aiMesh->mNumVertices;
	l_meshData->m_vertices.reserve(l_containerSize);

	for (auto i = (unsigned int)0; i < l_containerSize; i++)
	{
		Vertex l_Vertex;

		// positions
		if (&aiMesh->mVertices[i] != nullptr)
		{
			l_Vertex.m_pos.x = aiMesh->mVertices[i].x;
			l_Vertex.m_pos.y = aiMesh->mVertices[i].y;
			l_Vertex.m_pos.z = aiMesh->mVertices[i].z;
		}
		else
		{
			l_Vertex.m_pos.x = 0.0f;
			l_Vertex.m_pos.y = 0.0f;
			l_Vertex.m_pos.z = 0.0f;
		}

		// texture coordinates
		if (aiMesh->mTextureCoords[0])
		{
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			l_Vertex.m_texCoord.x = aiMesh->mTextureCoords[0][i].x;
			l_Vertex.m_texCoord.y = aiMesh->mTextureCoords[0][i].y;
		}
		else
		{
			l_Vertex.m_texCoord.x = 0.0f;
			l_Vertex.m_texCoord.y = 0.0f;
		}

		// normals
		if (aiMesh->mNormals)
		{
			l_Vertex.m_normal.x = aiMesh->mNormals[i].x;
			l_Vertex.m_normal.y = aiMesh->mNormals[i].y;
			l_Vertex.m_normal.z = aiMesh->mNormals[i].z;
		}
		else
		{
			l_Vertex.m_normal.x = 0.0f;
			l_Vertex.m_normal.y = 0.0f;
			l_Vertex.m_normal.z = 0.0f;
		}
		l_meshData->m_vertices.emplace_back(l_Vertex);
	}

	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	// @TODO: reserve fixed size vector
	for (auto i = (unsigned int)0; i < aiMesh->mNumFaces; i++)
	{
		aiFace face = aiMesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (auto j = (unsigned int)0; j < face.mNumIndices; j++)
		{
			l_meshData->m_indices.emplace_back(face.mIndices[j]);
		}
	}

	l_meshData->m_meshType = meshType::NORMAL;
	l_meshData->m_meshDrawMethod = meshDrawMethod;
	l_meshData->m_calculateNormals = caclNormal;
	l_meshData->m_calculateTangents = false;
	l_meshData->m_indicesSize = l_meshData->m_indices.size();

	l_meshData->m_objectStatus = objectStatus::STANDBY;
	g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_meshData);
}

/*
aiTextureType::aiTextureType_NORMALS textureType::NORMAL map_Kn normal map texture
aiTextureType::aiTextureType_DIFFUSE textureType::ALBEDO map_Kd albedo texture
aiTextureType::aiTextureType_SPECULAR textureType::METALLIC map_Ks metallic texture
aiTextureType::aiTextureType_AMBIENT textureType::ROUGHNESS map_Ka roughness texture
aiTextureType::aiTextureType_EMISSIVE textureType::AMBIENT_OCCLUSION map_emissive AO texture
*/

void InnoAssetSystem::processSingleAssimpMaterial(textureMap & textureMap, const aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod)
{
	for (auto i = (unsigned int)0; i < aiTextureType_UNKNOWN; i++)
	{
		if (aiMaterial->GetTextureCount(aiTextureType(i)) > 0)
		{
			auto l_texturePair = texturePair();

			aiString l_AssString;
			aiMaterial->GetTexture(aiTextureType(i), 0, &l_AssString);
			std::string l_localPath = std::string(l_AssString.C_Str());

			textureType l_textureType;

			if (aiTextureType(i) == aiTextureType::aiTextureType_NONE)
			{
				InnoLogSystem::printLog("AssetSystem: innoTexture: " + l_localPath + " is unknown type!");
				return;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_NORMALS)
			{
				l_textureType = textureType::NORMAL;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_DIFFUSE)
			{
				l_textureType = textureType::ALBEDO;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_SPECULAR)
			{
				l_textureType = textureType::METALLIC;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_AMBIENT)
			{
				l_textureType = textureType::ROUGHNESS;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_EMISSIVE)
			{
				l_textureType = textureType::AMBIENT_OCCLUSION;
			}
			else
			{
				InnoLogSystem::printLog("AssetSystem: innoTexture: " + l_localPath + " is unsupported type!");
				return;
			}
			// load image
			auto l_loadedTexture = loadTexture(l_localPath, l_textureType, textureWrapMethod);
			textureMap.emplace(l_loadedTexture);
		}
	}
}

int assimpMeshRawData::getNumVertices() const
{
	return m_aiMesh->mNumVertices;
}

int assimpMeshRawData::getNumFaces() const
{
	return m_aiMesh->mNumFaces;
}

int assimpMeshRawData::getNumIndicesInFace(int faceIndex) const
{
	return m_aiMesh->mFaces[faceIndex].mNumIndices;
}

vec4 assimpMeshRawData::getVertices(unsigned int index) const
{
	return vec4(m_aiMesh->mVertices[index].x, m_aiMesh->mVertices[index].y, m_aiMesh->mVertices[index].z, 1.0);
}

vec2 assimpMeshRawData::getTextureCoords(unsigned int index) const
{
	return vec2(m_aiMesh->mTextureCoords[0][index].x, m_aiMesh->mTextureCoords[0][index].y);
}

vec4 assimpMeshRawData::getNormals(unsigned int index) const
{
	return vec4(m_aiMesh->mNormals[index].x, m_aiMesh->mNormals[index].y, m_aiMesh->mNormals[index].z, 0.0);
}

int assimpMeshRawData::getIndices(int faceIndex, int index) const
{
	return m_aiMesh->mFaces[faceIndex].mIndices[index];
}
