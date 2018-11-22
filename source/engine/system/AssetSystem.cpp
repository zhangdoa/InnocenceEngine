#include "AssetSystem.h"

#include "../common/config.h"
#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include <experimental/filesystem>
#endif

#include "../common/ComponentHeaders.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "STB_Image/stb_image.h"
#include "../component/AssetSystemSingletonComponent.h"
#include "../component/GameSystemSingletonComponent.h"
#include "../component/PhysicsSystemSingletonComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoAssetSystemNS
{
	void loadDefaultAssets();
	void loadAssetsForComponents();

	modelMap loadModel(const std::string& fileName);
	modelMap loadModelFromDisk(const std::string & fileName);

	TextureDataComponent* loadTexture(const std::string& fileName, textureType textureType);
	TextureDataComponent* loadTextureFromDisk(const std::string& fileName, textureType textureType);

	modelMap processAssimpScene(const aiScene* aiScene);
	modelMap processAssimpNode(const aiNode * node, const aiScene * scene);
	modelPair processSingleAssimpMesh(const aiScene * scene, unsigned int meshIndex);
	MaterialDataComponent* processSingleAssimpMaterial(const aiMaterial * aiMaterial);

	MeshDataComponent* addMeshDataComponent();
	MaterialDataComponent* addMaterialDataComponent();
	TextureDataComponent* addTextureDataComponent();

	void assignUnitMesh(meshShapeType meshType, VisibleComponent* visibleComponent);

	void addUnitCube(MeshDataComponent& meshDataComponent);
	void addUnitSphere(MeshDataComponent& meshDataComponent);
	void addUnitQuad(MeshDataComponent& meshDataComponent);
	void addUnitLine(MeshDataComponent& meshDataComponent);

	static AssetSystemSingletonComponent* g_AssetSystemSingletonComponent;
	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
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

INNO_SYSTEM_EXPORT bool InnoAssetSystem::setup()
{
	InnoAssetSystemNS::g_AssetSystemSingletonComponent = &AssetSystemSingletonComponent::getInstance();
	InnoAssetSystemNS::g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();

	InnoAssetSystemNS::m_objectStatus = objectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::initialize()
{
	InnoAssetSystemNS::loadDefaultAssets();
	// @TODO: more granularly do IO operations

	InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_asyncTaskVector.push_back(g_pCoreSystem->getTaskSystem()->submit([]()
	{
		InnoAssetSystemNS::loadAssetsForComponents();
	}));

	g_pCoreSystem->getLogSystem()->printLog("AssetSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::terminate()
{
	InnoAssetSystemNS::m_objectStatus = objectStatus::STANDBY;
	InnoAssetSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog("AssetSystem has been terminated.");
	return true;
}

// @TODO: merge to a text file parser
std::string InnoAssetSystem::loadShader(const std::string & fileName)
{
	std::ifstream file;
	file.open((InnoAssetSystemNS:: g_AssetSystemSingletonComponent->m_shaderRelativePath + fileName).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	output = shaderStream.str();
	file.close();

	return output;
}

objectStatus InnoAssetSystem::getStatus()
{
	return InnoAssetSystemNS::m_objectStatus;
}

MeshDataComponent* InnoAssetSystemNS::addMeshDataComponent()
{
	auto newMesh = g_pCoreSystem->getMemorySystem()->spawn<MeshDataComponent>();
	auto l_parentEntity = InnoMath::createEntityID();
	newMesh->m_parentEntity = l_parentEntity;
	auto l_meshMap = &g_AssetSystemSingletonComponent->m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, MeshDataComponent*>(l_parentEntity, newMesh));
	return newMesh;
}

MaterialDataComponent* InnoAssetSystemNS::addMaterialDataComponent()
{
	auto newMaterial = g_pCoreSystem->getMemorySystem()->spawn<MaterialDataComponent>();
	auto l_parentEntity = InnoMath::createEntityID();
	newMaterial->m_parentEntity = l_parentEntity;
	auto l_materialMap = &g_AssetSystemSingletonComponent->m_materialMap;
	l_materialMap->emplace(std::pair<EntityID, MaterialDataComponent*>(l_parentEntity, newMaterial));
	return newMaterial;
}

TextureDataComponent* InnoAssetSystemNS::addTextureDataComponent()
{
	auto newTexture = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();
	auto l_parentEntity = InnoMath::createEntityID();
	newTexture->m_parentEntity = l_parentEntity;
	auto l_textureMap = &g_AssetSystemSingletonComponent->m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, TextureDataComponent*>(l_parentEntity, newTexture));
	return newTexture;
}

MeshDataComponent* InnoAssetSystem::getMeshDataComponent(EntityID EntityID)
{
	auto result = InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_meshMap.find(EntityID);
	if (result != InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("Error : AssetSystem : can't find MeshDataComponent by EntityID : " + std::to_string(EntityID) + " !");
		return nullptr;
	}
}

TextureDataComponent * InnoAssetSystem::getTextureDataComponent(EntityID EntityID)
{
	auto result = InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_textureMap.find(EntityID);
	if (result != InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("Error : AssetSystem : can't find TextureDataComponent by EntityID : " + std::to_string(EntityID) + " !");
		return nullptr;
	}
}

MeshDataComponent * InnoAssetSystem::getMeshDataComponent(meshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case meshShapeType::LINE:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_UnitLineTemplate; break;
	case meshShapeType::QUAD:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_UnitQuadTemplate; break;
	case meshShapeType::CUBE:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_UnitCubeTemplate; break;
	case meshShapeType::SPHERE:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_UnitSphereTemplate; break;
	case meshShapeType::CUSTOM:
		g_pCoreSystem->getLogSystem()->printLog("Error : AssetSystem : wrong meshShapeType passed to InnoAssetSystem::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoAssetSystem::getTextureDataComponent(textureType textureType)
{
	switch (textureType)
	{
	case textureType::INVISIBLE:
		return nullptr; break;
	case textureType::NORMAL:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_basicNormalTemplate; break;
	case textureType::ALBEDO:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_basicAlbedoTemplate; break;
	case textureType::METALLIC:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_basicMetallicTemplate; break;
	case textureType::ROUGHNESS:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_basicRoughnessTemplate; break;
	case textureType::AMBIENT_OCCLUSION:
		return InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_basicAOTemplate; break;
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

bool InnoAssetSystem::removeMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		g_pCoreSystem->getMemorySystem()->destroy<MeshDataComponent>(l_mesh->second);
		l_meshMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("Error : AssetSystem : can't remove MeshDataComponent by EntityID : " + std::to_string(EntityID) + " !");
		return false;
	}
}

bool InnoAssetSystem::removeTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			stbi_image_free(i);
		}
		
		g_pCoreSystem->getMemorySystem()->destroy<TextureDataComponent>(l_texture->second);
		l_textureMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("Error : AssetSystem : can't remove TextureDataComponent by EntityID : " + std::to_string(EntityID) + " !");
		return false;
	}

}

bool InnoAssetSystem::releaseRawDataForMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
	// @TODO:
		l_mesh->second->m_vertices.clear();
		l_mesh->second->m_vertices.shrink_to_fit();
		l_mesh->second->m_indices.clear();
		l_mesh->second->m_indices.shrink_to_fit();
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("Error : AssetSystem : can't release raw data for MeshDataComponent by EntityID : " + std::to_string(EntityID) + " !");
		return false;
	}

}

bool InnoAssetSystem::releaseRawDataForTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			stbi_image_free(i);
		}
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("Error : AssetSystem : can't release raw data for TextureDataComponent by EntityID : " + std::to_string(EntityID) + " !");
		return false;
	}

}

void InnoAssetSystemNS::addUnitCube(MeshDataComponent& meshDataComponent)
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = vec4(1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_5.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = vec4(1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_6.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = vec4(-1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_7.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_8.m_texCoord = vec2(0.0f, 1.0f);

	float vertices[] = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
	};

	for (size_t i = 0; i < 288; i += 8)
	{
		meshDataComponent.m_vertices.emplace_back(
			Vertex(
				vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0f),
				vec2(vertices[i + 6], vertices[i + 7]),
				vec4(vertices[i + 3], vertices[i + 4], vertices[i + 5], 0.0f)
		)
		);
	}
	
	//meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	//for (auto& l_vertexData : meshDataComponent.m_vertices)
	//{
	//	l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	//}

	for (unsigned int i = 0; i < 36; i++)
	{
		meshDataComponent.m_indices.emplace_back(i);
	}

	//meshDataComponent.m_indices = { 0, 3, 1, 1, 3, 2,
	//	4, 0, 5, 5, 0, 1,
	//	7, 4, 6, 6, 4, 5,
	//	3, 7, 2, 2, 7 ,6,
	//	4, 7, 0, 0, 7, 3,
	//	1, 2, 5, 5, 2, 6 };

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystemNS::addUnitSphere(MeshDataComponent& meshDataComponent)
{
	unsigned int X_SEGMENTS = 64;
	unsigned int Y_SEGMENTS = 64;
	auto l_containerSize = X_SEGMENTS * Y_SEGMENTS;
	meshDataComponent.m_vertices.reserve(l_containerSize);

	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = cos(xSegment * 2.0f * PI<float>) * sin(ySegment * PI<float>);
			float yPos = cos(ySegment * PI<float>);
			float zPos = sin(xSegment * 2.0f * PI<float>) * sin(ySegment * PI<float>);

			Vertex l_VertexData;
			l_VertexData.m_pos = vec4(xPos, yPos, zPos, 1.0f);
			l_VertexData.m_texCoord = vec2(xSegment, ySegment);
			l_VertexData.m_normal = vec4(xPos, yPos, zPos, 0.0f).normalize();
			meshDataComponent.m_vertices.emplace_back(l_VertexData);
		}
	}

	bool oddRow = false;
	for (unsigned y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned x = 0; x <= X_SEGMENTS; ++x)
			{
				meshDataComponent.m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
				meshDataComponent.m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				meshDataComponent.m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				meshDataComponent.m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystemNS::addUnitQuad(MeshDataComponent& meshDataComponent)
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4 };
	meshDataComponent.m_indices = { 0, 1, 3, 1, 2, 3 };
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystemNS::addUnitLine(MeshDataComponent& meshDataComponent)
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(0.0f, 0.0f);

	meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2 };
	meshDataComponent.m_indices = { 0, 1 };
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystemNS::loadDefaultAssets()
{
	g_AssetSystemSingletonComponent->m_basicNormalTemplate = loadTextureFromDisk("basic_normal.png", textureType::NORMAL);
	g_AssetSystemSingletonComponent->m_basicAlbedoTemplate = loadTextureFromDisk("basic_albedo.png", textureType::ALBEDO);
	g_AssetSystemSingletonComponent->m_basicMetallicTemplate = loadTextureFromDisk("basic_metallic.png", textureType::METALLIC);
	g_AssetSystemSingletonComponent->m_basicRoughnessTemplate = loadTextureFromDisk("basic_roughness.png", textureType::ROUGHNESS);
	g_AssetSystemSingletonComponent->m_basicAOTemplate = loadTextureFromDisk("basic_ao.png", textureType::AMBIENT_OCCLUSION);

	g_AssetSystemSingletonComponent->m_UnitLineTemplate = addMeshDataComponent();
	auto lastLineMeshData = g_AssetSystemSingletonComponent->m_UnitLineTemplate;
	InnoAssetSystemNS::addUnitLine(*lastLineMeshData);
	lastLineMeshData->m_meshType = meshType::NORMAL;
	lastLineMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	lastLineMeshData->m_objectStatus = objectStatus::STANDBY;

	g_AssetSystemSingletonComponent->m_UnitQuadTemplate = addMeshDataComponent();
	auto lastQuadMeshData = g_AssetSystemSingletonComponent->m_UnitQuadTemplate;
	InnoAssetSystemNS::addUnitQuad(*lastQuadMeshData);
	lastQuadMeshData->m_meshType = meshType::NORMAL;
	lastQuadMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	lastQuadMeshData->m_objectStatus = objectStatus::STANDBY;

	g_AssetSystemSingletonComponent->m_UnitCubeTemplate = addMeshDataComponent();
	auto lastCubeMeshData = g_AssetSystemSingletonComponent->m_UnitCubeTemplate;
	InnoAssetSystemNS::addUnitCube(*lastCubeMeshData);
	lastCubeMeshData->m_meshType = meshType::NORMAL;
	lastCubeMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	lastCubeMeshData->m_objectStatus = objectStatus::STANDBY;

	g_AssetSystemSingletonComponent->m_UnitSphereTemplate = addMeshDataComponent();
	auto lastSphereMeshData = g_AssetSystemSingletonComponent->m_UnitSphereTemplate;
	InnoAssetSystemNS::addUnitSphere(*lastSphereMeshData);
	lastSphereMeshData->m_meshType = meshType::NORMAL;
	lastSphereMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	lastSphereMeshData->m_objectStatus = objectStatus::STANDBY;
}

void InnoAssetSystemNS::loadAssetsForComponents()
{
	//for (auto& l_cameraComponent : InnoAssetSystemNS::g_GameSystemSingletonComponent->m_CameraComponents)
	//{
	//	if (l_cameraComponent->m_drawAABB)
	//	{
	//		auto l_Mesh = InnoAssetSystemNS::addMeshDataComponent();
	//		l_Mesh->m_vertices = l_cameraComponent->m_AABB.m_vertices;
	//		l_Mesh->m_indices = l_cameraComponent->m_AABB.m_indices;
	//		l_Mesh->m_indicesSize = l_Mesh->m_indices.size();
	//		l_Mesh->m_objectStatus = objectStatus::STANDBY;
	//		g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_Mesh);
	//		l_cameraComponent->m_AABBMeshID = l_Mesh->m_parentEntity;
	//	}
	//	if (l_cameraComponent->m_drawFrustum)
	//	{
	//		auto l_Mesh = InnoAssetSystemNS::addMeshDataComponent();
	//		l_Mesh->m_vertices = l_cameraComponent->m_frustumVertices;
	//		l_Mesh->m_indices = l_cameraComponent->m_frustumIndices;
	//		l_Mesh->m_indicesSize = l_Mesh->m_indices.size();
	//		l_Mesh->m_objectStatus = objectStatus::STANDBY;
	//		g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_Mesh);
	//		l_cameraComponent->m_FrustumMeshID = l_Mesh->m_parentEntity;
	//	}
	//}
	//for (auto& l_lightComponent : InnoAssetSystemNS::g_GameSystemSingletonComponent->m_LightComponents)
	//{
	//	if (l_lightComponent->m_drawAABB)
	//	{
	//		auto l_containerSize = l_lightComponent->m_AABBs.size();
	//		l_lightComponent->m_AABBMeshIDs.reserve(l_containerSize);
	//		
	//		for (size_t i = 0; i < l_containerSize; i++)
	//		{
	//			auto l_Mesh = InnoAssetSystemNS::addMeshDataComponent();
	//			l_Mesh->m_vertices = l_lightComponent->m_AABBs[i].m_vertices;
	//			l_Mesh->m_indices = l_lightComponent->m_AABBs[i].m_indices;
	//			l_Mesh->m_indicesSize = l_Mesh->m_indices.size();
	//			l_Mesh->m_objectStatus = objectStatus::STANDBY;
	//			g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_Mesh);
	//			l_lightComponent->m_AABBMeshIDs.emplace_back(l_Mesh->m_parentEntity);
	//		}
	//	}
	//}
	for (auto& l_environmentCaptureComponent : InnoAssetSystemNS::g_GameSystemSingletonComponent->m_EnvironmentCaptureComponents)
	{
		l_environmentCaptureComponent->m_TDC = InnoAssetSystemNS::loadTexture(l_environmentCaptureComponent->m_cubemapTextureFileName, textureType::EQUIRETANGULAR);
	}
	for (auto& l_visibleComponent : InnoAssetSystemNS::g_GameSystemSingletonComponent->m_VisibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType != visiblilityType::INVISIBLE)
		{
			if (l_visibleComponent->m_meshShapeType == meshShapeType::CUSTOM)
			{
				if (l_visibleComponent->m_modelFileName != "")
				{
					l_visibleComponent->m_modelMap = InnoAssetSystemNS::loadModel(l_visibleComponent->m_modelFileName);
				}
			}
			else
			{
				assignUnitMesh(l_visibleComponent->m_meshShapeType, l_visibleComponent);
			}
			PhysicsSystemSingletonComponent::getInstance().m_uninitializedVisibleComponents.push(l_visibleComponent);
			//if (l_visibleComponent->m_drawAABB)
			//{
			//	auto l_Mesh = InnoAssetSystemNS::addMeshDataComponent();
			//	l_Mesh->m_vertices = l_visibleComponent->m_AABB.m_vertices;
			//	l_Mesh->m_indices = l_visibleComponent->m_AABB.m_indices;
			//	l_Mesh->m_indicesSize = l_Mesh->m_indices.size();
			//	l_Mesh->m_objectStatus = objectStatus::STANDBY;
			//	g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_Mesh);
			//	l_visibleComponent->m_AABBMeshID = l_Mesh->m_parentEntity;
			//}
		}
	}
}

void InnoAssetSystemNS::assignUnitMesh(meshShapeType meshType, VisibleComponent* visibleComponent)
{
    MeshDataComponent* l_UnitMeshTemplate;
    switch (meshType)
    {
            case meshShapeType::LINE: l_UnitMeshTemplate = InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_UnitLineTemplate; break;
            case meshShapeType::QUAD: l_UnitMeshTemplate = InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_UnitQuadTemplate; break;
            case meshShapeType::CUBE: l_UnitMeshTemplate = InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_UnitCubeTemplate; break;
            case meshShapeType::SPHERE: l_UnitMeshTemplate = InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_UnitSphereTemplate; break;
            case meshShapeType::CUSTOM: break;
    }
	visibleComponent->m_modelMap.emplace(l_UnitMeshTemplate, addMaterialDataComponent());
}

modelMap InnoAssetSystemNS::loadModel(const std::string & fileName)
{
	modelMap l_result;
	// check if this file has already been loaded once
	auto l_loadedmodelMap = InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_loadedModelMap.find(fileName);
	if (l_loadedmodelMap != InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_loadedModelMap.end())
	{
		g_pCoreSystem->getLogSystem()->printLog("AssetSystem: innoMesh: " + fileName + " is already loaded.");
		for (auto& i : l_loadedmodelMap->second)
		{
			auto l_material = addMaterialDataComponent();
			*l_material = *i.second;
			l_result.emplace(i.first, l_material);
		}
		return l_result;
	}
	else
	{
		auto l_loadedModelMap = loadModelFromDisk(fileName);
		//mark as loaded
		InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_loadedModelMap.emplace(fileName, l_loadedModelMap);
		return l_loadedModelMap;		
	}
}

modelMap InnoAssetSystemNS::loadModelFromDisk(const std::string & fileName)
{
	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene;
#if defined INNO_PLATFORM_WIN32 || defined INNO_PLATFORM_WIN64
	if (std::experimental::filesystem::exists(std::experimental::filesystem::path(InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_modelRelativePath + fileName)))
	{
		l_assScene = l_assImporter.ReadFile(InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs);
		// @TODO: serilization
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("AssetSystem: " + fileName + " doesn't exist!");
		return modelMap();
	}
#else
	l_assScene = l_assImporter.ReadFile(InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs);
	if (l_assScene == nullptr)
	{
		g_pCoreSystem->getLogSystem()->printLog("AssetSystem: " + fileName + " doesn't exist!");
		return modelMap();
	}
#endif
	if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		g_pCoreSystem->getLogSystem()->printLog("Error : AssetSystem : ASSIMP : " + std::string{ l_assImporter.GetErrorString() });
		return modelMap();
	}

	auto l_loadedModelMap = processAssimpScene(l_assScene);

	g_pCoreSystem->getLogSystem()->printLog("AssetSystem: " + fileName + " is loaded for the first time.");

	return l_loadedModelMap;
}

modelMap InnoAssetSystemNS::processAssimpScene(const aiScene* aiScene)
{
	auto l_loadedModelMapInScene = modelMap();
	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		auto l_loadedModelMapInNode = processAssimpNode(aiScene->mRootNode, aiScene);
		for (auto pair : l_loadedModelMapInNode)
		{
			l_loadedModelMapInScene.emplace(pair);
		}
	}
	for (auto i = (unsigned int)0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			auto l_loadedModelMapInNode = processAssimpNode(aiScene->mRootNode->mChildren[i], aiScene);
			for (auto pair : l_loadedModelMapInNode)
			{
				l_loadedModelMapInScene.emplace(pair);
			}
		}
	}
	return l_loadedModelMapInScene;
}

modelMap InnoAssetSystemNS::processAssimpNode(const aiNode * node, const aiScene * scene)
{
	auto l_loadedModelMap = modelMap();
	// process each mesh located at the current node
	for (auto i = (unsigned int)0; i < node->mNumMeshes; i++)
	{
		auto l_modelPair = processSingleAssimpMesh(scene, node->mMeshes[i]);
		l_loadedModelMap.emplace(l_modelPair);
	}

	return l_loadedModelMap;
}

modelPair InnoAssetSystemNS::processSingleAssimpMesh(const aiScene * scene, unsigned int meshIndex)
{
	auto l_aiMesh = scene->mMeshes[meshIndex];

	auto l_meshData = addMeshDataComponent();
	auto l_verticesNumber = l_aiMesh->mNumVertices;
	l_meshData->m_vertices.reserve(l_verticesNumber);

	for (auto i = (unsigned int)0; i < l_verticesNumber; i++)
	{
		Vertex l_Vertex;

		// positions
		if (&l_aiMesh->mVertices[i] != nullptr)
		{
			l_Vertex.m_pos.x = l_aiMesh->mVertices[i].x;
			l_Vertex.m_pos.y = l_aiMesh->mVertices[i].y;
			l_Vertex.m_pos.z = l_aiMesh->mVertices[i].z;
		}
		else
		{
			l_Vertex.m_pos.x = 0.0f;
			l_Vertex.m_pos.y = 0.0f;
			l_Vertex.m_pos.z = 0.0f;
		}

		// texture coordinates
		if (l_aiMesh->mTextureCoords[0])
		{
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			l_Vertex.m_texCoord.x = l_aiMesh->mTextureCoords[0][i].x;
			l_Vertex.m_texCoord.y = l_aiMesh->mTextureCoords[0][i].y;
		}
		else
		{
			l_Vertex.m_texCoord.x = 0.0f;
			l_Vertex.m_texCoord.y = 0.0f;
		}

		// normals
		if (l_aiMesh->mNormals)
		{
			l_Vertex.m_normal.x = l_aiMesh->mNormals[i].x;
			l_Vertex.m_normal.y = l_aiMesh->mNormals[i].y;
			l_Vertex.m_normal.z = l_aiMesh->mNormals[i].z;
		}
		else
		{
			l_Vertex.m_normal.x = 0.0f;
			l_Vertex.m_normal.y = 0.0f;
			l_Vertex.m_normal.z = 0.0f;
		}
		l_meshData->m_vertices.emplace_back(l_Vertex);
	}

	l_meshData->m_vertices.shrink_to_fit();

	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	// @TODO: reserve fixed size vector

	for (auto i = (unsigned int)0; i < l_aiMesh->mNumFaces; i++)
	{
		aiFace l_face = l_aiMesh->mFaces[i];
		l_meshData->m_indicesSize += l_face.mNumIndices;
	}
	l_meshData->m_indices.reserve(l_meshData->m_indicesSize);

	for (auto i = (unsigned int)0; i < l_aiMesh->mNumFaces; i++)
	{
		aiFace l_face = l_aiMesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (auto j = (unsigned int)0; j < l_face.mNumIndices; j++)
		{
			l_meshData->m_indices.emplace_back(l_face.mIndices[j]);
		}
	}
	l_meshData->m_indices.shrink_to_fit();

	auto l_modelPair = modelPair();
	l_modelPair.first = l_meshData;

	// process material
	if (l_aiMesh->mMaterialIndex > 0)
	{
		l_modelPair.second = processSingleAssimpMaterial(scene->mMaterials[l_aiMesh->mMaterialIndex]);
	}
	else
	{
		l_modelPair.second = addMaterialDataComponent();
	}

	l_meshData->m_objectStatus = objectStatus::STANDBY;
	g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_meshData);

	return l_modelPair;
}

/*
aiTextureType::aiTextureType_NORMALS textureType::NORMAL map_Kn normal map texture
aiTextureType::aiTextureType_DIFFUSE textureType::ALBEDO map_Kd albedo texture
aiTextureType::aiTextureType_SPECULAR textureType::METALLIC map_Ks metallic texture
aiTextureType::aiTextureType_AMBIENT textureType::ROUGHNESS map_Ka roughness texture
aiTextureType::aiTextureType_EMISSIVE textureType::AMBIENT_OCCLUSION map_emissive AO texture
*/

MaterialDataComponent* InnoAssetSystemNS::processSingleAssimpMaterial(const aiMaterial * aiMaterial)
{
	auto l_loadedMaterialDataComponent = addMaterialDataComponent();

	for (auto i = (unsigned int)0; i < aiTextureType_UNKNOWN; i++)
	{
		if (aiMaterial->GetTextureCount(aiTextureType(i)) > 0)
		{
			aiString l_AssString;
			aiMaterial->GetTexture(aiTextureType(i), 0, &l_AssString);
			std::string l_localPath = std::string(l_AssString.C_Str());
			
			if (aiTextureType(i) == aiTextureType::aiTextureType_NONE)
			{
				g_pCoreSystem->getLogSystem()->printLog("AssetSystem: " + l_localPath + " is unknown texture type!");
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_NORMALS)
			{
				l_loadedMaterialDataComponent->m_texturePack.m_normalTDC.second = loadTexture(l_localPath, textureType::NORMAL);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_DIFFUSE)
			{
				l_loadedMaterialDataComponent->m_texturePack.m_albedoTDC.second = loadTexture(l_localPath, textureType::ALBEDO);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_SPECULAR)
			{
				l_loadedMaterialDataComponent->m_texturePack.m_metallicTDC.second = loadTexture(l_localPath, textureType::METALLIC);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_AMBIENT)
			{
				l_loadedMaterialDataComponent->m_texturePack.m_roughnessTDC.second = loadTexture(l_localPath, textureType::ROUGHNESS);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_EMISSIVE)
			{
				l_loadedMaterialDataComponent->m_texturePack.m_aoTDC.second = loadTexture(l_localPath, textureType::AMBIENT_OCCLUSION);
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog("AssetSystem: " + l_localPath + " is unsupported texture type!");
			}
		}
	}

	// albedo (RGB) + Metallic + Roughness + AO + 2 additional data
	auto l_meshColor = meshColor();
	auto l_result = aiColor3D();
	if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_meshColor.albedo_r = l_result.r;
		l_meshColor.albedo_g = l_result.g;
		l_meshColor.albedo_b = l_result.b;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_meshColor.metallic = l_result.r;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_meshColor.roughness = l_result.r;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_meshColor.ao = l_result.r;
	}

	l_loadedMaterialDataComponent->m_meshColor = l_meshColor;

	return l_loadedMaterialDataComponent;
}

TextureDataComponent* InnoAssetSystemNS::loadTexture(const std::string& fileName, textureType textureType)
{
	auto l_loadedTDC = InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_loadedTextureMap.find(fileName);
	if (l_loadedTDC != InnoAssetSystemNS::g_AssetSystemSingletonComponent->m_loadedTextureMap.end())
	{
		g_pCoreSystem->getLogSystem()->printLog("AssetSystem: innoTexture: " + fileName + " is already loaded.");
		return l_loadedTDC->second;
	}
	else
	{
		auto l_TDC = InnoAssetSystemNS::loadTextureFromDisk(fileName, textureType);

		g_AssetSystemSingletonComponent->m_loadedTextureMap.emplace(fileName, l_TDC);

		return l_TDC;
	}

}

TextureDataComponent* InnoAssetSystemNS::loadTextureFromDisk(const std::string& fileName, textureType textureType)
{
	auto l_TDC = InnoAssetSystemNS::addTextureDataComponent();
	int width, height, nrChannels;

	// load image, flip texture
	stbi_set_flip_vertically_on_load(true);

	if (textureType == textureType::EQUIRETANGULAR)
	{
		auto *data = stbi_loadf((g_AssetSystemSingletonComponent->m_textureRelativePath + fileName).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			l_TDC->m_textureDataDesc.textureType = textureType::EQUIRETANGULAR;
			l_TDC->m_textureDataDesc.textureColorComponentsFormat = textureColorComponentsFormat((unsigned int)textureColorComponentsFormat::R16F + (nrChannels - 1));
			l_TDC->m_textureDataDesc.texturePixelDataFormat = texturePixelDataFormat(nrChannels - 1);
			l_TDC->m_textureDataDesc.textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
			l_TDC->m_textureDataDesc.textureMinFilterMethod = textureFilterMethod::LINEAR;
			l_TDC->m_textureDataDesc.textureMagFilterMethod = textureFilterMethod::LINEAR;
			l_TDC->m_textureDataDesc.textureWidth = width;
			l_TDC->m_textureDataDesc.textureHeight = height;
			l_TDC->m_textureDataDesc.texturePixelDataType = texturePixelDataType::FLOAT;
			l_TDC->m_textureData = { data };
			l_TDC->m_objectStatus = objectStatus::STANDBY;
			g_AssetSystemSingletonComponent->m_uninitializedTextureComponents.push(l_TDC);

			g_pCoreSystem->getLogSystem()->printLog("AssetSystem: " + fileName + " is loaded.");
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog("Error: AssetSystem: STB_Image: Failed to load texture: " + (g_AssetSystemSingletonComponent->m_textureRelativePath + fileName));
		}
	}
	else
	{
		auto *data = stbi_load((g_AssetSystemSingletonComponent->m_textureRelativePath + fileName).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			l_TDC->m_textureDataDesc.textureType = textureType;
			l_TDC->m_textureDataDesc.textureColorComponentsFormat = textureColorComponentsFormat(nrChannels - 1);
			l_TDC->m_textureDataDesc.texturePixelDataFormat = texturePixelDataFormat(nrChannels - 1);
			l_TDC->m_textureDataDesc.textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
			l_TDC->m_textureDataDesc.textureMinFilterMethod = textureFilterMethod::LINEAR_MIPMAP_LINEAR;
			l_TDC->m_textureDataDesc.textureMagFilterMethod = textureFilterMethod::LINEAR;
			l_TDC->m_textureDataDesc.textureWidth = width;
			l_TDC->m_textureDataDesc.textureHeight = height;
			l_TDC->m_textureDataDesc.texturePixelDataType = texturePixelDataType::UNSIGNED_BYTE;
			l_TDC->m_textureData = { data };
			l_TDC->m_objectStatus = objectStatus::STANDBY;
			g_AssetSystemSingletonComponent->m_uninitializedTextureComponents.push(l_TDC);

			g_pCoreSystem->getLogSystem()->printLog("AssetSystem: " + fileName + " is loaded.");
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog("Error: AssetSystem: STB_Image: Failed to load texture: " + (g_AssetSystemSingletonComponent->m_textureRelativePath + fileName));
		}
	}
	return l_TDC;
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