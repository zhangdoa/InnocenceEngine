#include "AssetSystem.h"
#include "../common/ComponentHeaders.h"

namespace fs = std::filesystem;

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoAssetSystemNS
{
	bool initializeComponentPool();

	void loadFolderData();
	void loadDefaultAssets();
	void loadAssetsForComponents();

	ModelMap loadModel(const std::string& fileName);
	TextureDataComponent* loadTexture(const std::string& fileName, TextureSamplerType textureSamplerType, TextureUsageType textureUsageType);

	MeshDataComponent* addMeshDataComponent();
	MaterialDataComponent* addMaterialDataComponent();
	TextureDataComponent* addTextureDataComponent();

	void assignUnitMesh(MeshShapeType MeshUsageType, VisibleComponent* visibleComponent);

	void addUnitCube(MeshDataComponent& meshDataComponent);
	void addUnitSphere(MeshDataComponent& meshDataComponent);
	void addUnitQuad(MeshDataComponent& meshDataComponent);
	void addUnitLine(MeshDataComponent& meshDataComponent);
	void addTerrain(MeshDataComponent& meshDataComponent);

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	std::vector<InnoFuture<void>> m_asyncTask;

	MeshDataComponent* m_UnitLineMDC;
	MeshDataComponent* m_UnitQuadMDC;
	MeshDataComponent* m_UnitCubeMDC;
	MeshDataComponent* m_UnitSphereMDC;
	MeshDataComponent* m_TerrainMDC;

	TextureDataComponent* m_basicNormalTDC;
	TextureDataComponent* m_basicAlbedoTDC;
	TextureDataComponent* m_basicMetallicTDC;
	TextureDataComponent* m_basicRoughnessTDC;
	TextureDataComponent* m_basicAOTDC;

	TextureDataComponent* m_iconTemplate_OBJ;
	TextureDataComponent* m_iconTemplate_PNG;
	TextureDataComponent* m_iconTemplate_SHADER;
	TextureDataComponent* m_iconTemplate_UNKNOWN;

	TextureDataComponent* m_iconTemplate_DirectionalLight;
	TextureDataComponent* m_iconTemplate_PointLight;
	TextureDataComponent* m_iconTemplate_SphereLight;

	DirectoryMetadata m_rootDirectoryMetadata;

	std::unordered_map<EntityID, MeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, MaterialDataComponent*> m_materialMap;
	std::unordered_map<EntityID, TextureDataComponent*> m_textureMap;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;
}

bool InnoAssetSystemNS::initializeComponentPool()
{
	m_MeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MeshDataComponent), 16384);
	m_MaterialDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MaterialDataComponent), 32768);
	m_TextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(TextureDataComponent), 32768);

	return true;
}
INNO_SYSTEM_EXPORT bool InnoAssetSystem::setup()
{
	InnoAssetSystemNS::initializeComponentPool();

	InnoAssetSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::terminate()
{
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::STANDBY;
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem has been terminated.");
	return true;
}

ObjectStatus InnoAssetSystem::getStatus()
{
	return InnoAssetSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT void InnoAssetSystem::loadDefaultAssets()
{
	InnoAssetSystemNS::loadDefaultAssets();
}

INNO_SYSTEM_EXPORT void InnoAssetSystem::loadAssetsForComponents()
{
	InnoAssetSystemNS::loadAssetsForComponents();
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoAssetSystem::addMeshDataComponent()
{
	return InnoAssetSystemNS::addMeshDataComponent();
}

INNO_SYSTEM_EXPORT MaterialDataComponent * InnoAssetSystem::addMaterialDataComponent()
{
	return InnoAssetSystemNS::addMaterialDataComponent();
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::addTextureDataComponent()
{
	return InnoAssetSystemNS::addTextureDataComponent();
}

MeshDataComponent* InnoAssetSystemNS::addMeshDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(MeshDataComponent));
	auto l_MDC = new(l_rawPtr)MeshDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_meshMap = &m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, MeshDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

MaterialDataComponent* InnoAssetSystemNS::addMaterialDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(MaterialDataComponent));
	auto l_MDC = new(l_rawPtr)MaterialDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_materialMap = &m_materialMap;
	l_materialMap->emplace(std::pair<EntityID, MaterialDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

TextureDataComponent* InnoAssetSystemNS::addTextureDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(TextureDataComponent));
	auto l_TDC = new(l_rawPtr)TextureDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_TDC->m_parentEntity = l_parentEntity;
	auto l_textureMap = &m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, TextureDataComponent*>(l_parentEntity, l_TDC));
	return l_TDC;
}

MeshDataComponent* InnoAssetSystem::getMeshDataComponent(EntityID EntityID)
{
	auto result = InnoAssetSystemNS::m_meshMap.find(EntityID);
	if (result != InnoAssetSystemNS::m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: can't find MeshDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

TextureDataComponent * InnoAssetSystem::getTextureDataComponent(EntityID EntityID)
{
	auto result = InnoAssetSystemNS::m_textureMap.find(EntityID);
	if (result != InnoAssetSystemNS::m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: can't find TextureDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

MeshDataComponent * InnoAssetSystem::getMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return InnoAssetSystemNS::m_UnitLineMDC; break;
	case MeshShapeType::QUAD:
		return InnoAssetSystemNS::m_UnitQuadMDC; break;
	case MeshShapeType::CUBE:
		return InnoAssetSystemNS::m_UnitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return InnoAssetSystemNS::m_UnitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return InnoAssetSystemNS::m_TerrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: wrong MeshShapeType passed to InnoAssetSystem::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoAssetSystem::getTextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return InnoAssetSystemNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return InnoAssetSystemNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return InnoAssetSystemNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return InnoAssetSystemNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return InnoAssetSystemNS::m_basicAOTDC; break;
	case TextureUsageType::RENDER_TARGET:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

INNO_SYSTEM_EXPORT TextureDataComponent* InnoAssetSystem::getTextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return InnoAssetSystemNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return InnoAssetSystemNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return InnoAssetSystemNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return InnoAssetSystemNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return InnoAssetSystemNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return InnoAssetSystemNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return InnoAssetSystemNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

bool InnoAssetSystem::removeMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &InnoAssetSystemNS::m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		g_pCoreSystem->getMemorySystem()->destroyObject(InnoAssetSystemNS::m_MeshDataComponentPool, sizeof(MeshDataComponent), l_mesh->second);
		l_meshMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: can't remove MeshDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

bool InnoAssetSystem::removeTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &InnoAssetSystemNS::m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			// @TODO
		}

		g_pCoreSystem->getMemorySystem()->destroyObject(InnoAssetSystemNS::m_TextureDataComponentPool, sizeof(TextureDataComponent), l_texture->second);
		l_textureMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: can't remove TextureDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

bool InnoAssetSystem::releaseRawDataForMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &InnoAssetSystemNS::m_meshMap;
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
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: can't release raw data for MeshDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

bool InnoAssetSystem::releaseRawDataForTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &InnoAssetSystemNS::m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			// @TODO:
		}
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: can't release raw data for TextureDataComponent by EntityID : " + EntityID + " !");
		return false;
	}
}

INNO_SYSTEM_EXPORT DirectoryMetadata* InnoAssetSystem::getRootDirectoryMetadata()
{
	return &InnoAssetSystemNS::m_rootDirectoryMetadata;
}

void InnoAssetSystemNS::addUnitCube(MeshDataComponent& meshDataComponent)
{
	float vertices[] = {
		// positions     // normals      // texture coords
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

		-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
	};

	meshDataComponent.m_vertices.reserve(288);

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

	meshDataComponent.m_indices.reserve(36);

	for (unsigned int i = 0; i < 36; i++)
	{
		meshDataComponent.m_indices.emplace_back(i);
	}

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
				meshDataComponent.m_indices.push_back(y    * (X_SEGMENTS + 1) + x);
				meshDataComponent.m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				meshDataComponent.m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				meshDataComponent.m_indices.push_back(y    * (X_SEGMENTS + 1) + x);
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

void InnoAssetSystemNS::addTerrain(MeshDataComponent& meshDataComponent)
{
	auto l_gridSize = 256;
	auto l_gridSize2 = l_gridSize * l_gridSize;
	auto l_gridSizehalf = l_gridSize / 2;
	meshDataComponent.m_vertices.reserve(l_gridSize2 * 4);
	meshDataComponent.m_indices.reserve(l_gridSize2 * 6);

	for (auto j = 0; j < l_gridSize; j++)
	{
		for (auto i = 0; i < l_gridSize; i++)
		{
			Vertex l_VertexData_1;
			l_VertexData_1.m_pos = vec4((float)(i - l_gridSizehalf), 0.0f, (float)(j - l_gridSizehalf), 1.0f);
			l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_1);

			Vertex l_VertexData_2;
			l_VertexData_2.m_pos = vec4((float)(i - l_gridSizehalf), 0.0f, (float)(j - l_gridSizehalf + 1), 1.0f);
			l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_2);

			Vertex l_VertexData_3;
			l_VertexData_3.m_pos = vec4((float)(i - l_gridSizehalf + 1), 0.0f, (float)(j - l_gridSizehalf + 1), 1.0f);
			l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_3);

			Vertex l_VertexData_4;
			l_VertexData_4.m_pos = vec4((float)(i - l_gridSizehalf + 1), 0.0f, (float)(j - l_gridSizehalf), 1.0f);
			l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_4);

			auto l_gridIndex = 4 * (i)+4 * l_gridSize * (j);
			meshDataComponent.m_indices.emplace_back(0 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(1 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(3 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(1 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(2 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(3 + l_gridIndex);
		}
	}
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystemNS::loadFolderData()
{
	std::function<FileExplorerIconType(const std::string&)> getIconType =
		[&](const std::string& extension) -> FileExplorerIconType {
		if (extension == ".obj")
		{
			return FileExplorerIconType::OBJ;
		}
		else if (extension == ".png")
		{
			return FileExplorerIconType::PNG;
		}
		else if (extension == ".sf")
		{
			return FileExplorerIconType::SHADER;
		}
		else
		{
			return FileExplorerIconType::UNKNOWN;
		}
	};

	auto path = "..//res";

	std::function<void(const fs::path& pathToShow, int level, DirectoryMetadata* parentDirectoryMetadata)> f_directoryTreeBuilder =
		[&](const fs::path& pathToShow, int level, DirectoryMetadata* parentDirectoryMetadata) {
		if (fs::exists(pathToShow) && fs::is_directory(pathToShow))
		{
			for (const auto& entry : fs::directory_iterator(pathToShow))
			{
				if (fs::is_directory(entry.status()))
				{
					DirectoryMetadata l_directoryMetadata;
					l_directoryMetadata.depth = level + 1;
					l_directoryMetadata.directoryName = entry.path().stem().generic_string();
					f_directoryTreeBuilder(entry, level + 1, &l_directoryMetadata);
					parentDirectoryMetadata->childrenDirectories.emplace_back(l_directoryMetadata);
				}
				else if (fs::is_regular_file(entry.status()))
				{
					AssetMetadata l_assetMetadata;
					l_assetMetadata.fullPath = entry.path().generic_string();
					l_assetMetadata.fileName = entry.path().stem().generic_string();
					l_assetMetadata.extension = entry.path().extension().generic_string();
					l_assetMetadata.iconType = getIconType(l_assetMetadata.extension);
					parentDirectoryMetadata->childrenAssets.emplace_back(l_assetMetadata);
				}
			}
		}
	};

	std::function<void(DirectoryMetadata* directoryMetadata)> f_assignParentDirectory =
		[&](DirectoryMetadata* directoryMetadata) {
		for (auto& i : directoryMetadata->childrenDirectories)
		{
			i.parentDirectory = directoryMetadata;
			f_assignParentDirectory(&i);
		}
	};

	f_directoryTreeBuilder(path, 0, &m_rootDirectoryMetadata);
	f_assignParentDirectory(&m_rootDirectoryMetadata);
}

void InnoAssetSystemNS::loadDefaultAssets()
{
	InnoAssetSystemNS::loadFolderData();

	m_basicNormalTDC = loadTexture("..//res//textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_basicAlbedoTDC = loadTexture("..//res//textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	m_basicMetallicTDC = loadTexture("..//res//textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	m_basicRoughnessTDC = loadTexture("..//res//textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	m_basicAOTDC = loadTexture("..//res//textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	m_iconTemplate_OBJ = loadTexture("..//res//textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_PNG = loadTexture("..//res//textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_SHADER = loadTexture("..//res//textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_UNKNOWN = loadTexture("..//res//textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_iconTemplate_DirectionalLight = loadTexture("..//res//textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_PointLight = loadTexture("..//res//textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_SphereLight = loadTexture("..//res//textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_UnitLineMDC = addMeshDataComponent();
	auto lastLineMeshData = m_UnitLineMDC;
	InnoAssetSystemNS::addUnitLine(*lastLineMeshData);
	lastLineMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastLineMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	lastLineMeshData->m_meshShapeType = MeshShapeType::LINE;
	lastLineMeshData->m_objectStatus = ObjectStatus::STANDBY;

	m_UnitQuadMDC = addMeshDataComponent();
	auto lastQuadMeshData = m_UnitQuadMDC;
	InnoAssetSystemNS::addUnitQuad(*lastQuadMeshData);
	lastQuadMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastQuadMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	lastLineMeshData->m_meshShapeType = MeshShapeType::QUAD;
	lastQuadMeshData->m_objectStatus = ObjectStatus::STANDBY;

	m_UnitCubeMDC = addMeshDataComponent();
	auto lastCubeMeshData = m_UnitCubeMDC;
	InnoAssetSystemNS::addUnitCube(*lastCubeMeshData);
	lastCubeMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastCubeMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	lastLineMeshData->m_meshShapeType = MeshShapeType::CUBE;
	lastCubeMeshData->m_objectStatus = ObjectStatus::STANDBY;

	m_UnitSphereMDC = addMeshDataComponent();
	auto lastSphereMeshData = m_UnitSphereMDC;
	InnoAssetSystemNS::addUnitSphere(*lastSphereMeshData);
	lastSphereMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastSphereMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	lastLineMeshData->m_meshShapeType = MeshShapeType::SPHERE;
	lastSphereMeshData->m_objectStatus = ObjectStatus::STANDBY;

	m_TerrainMDC = addMeshDataComponent();
	auto lastTerrainMeshData = m_TerrainMDC;
	InnoAssetSystemNS::addTerrain(*lastTerrainMeshData);
	lastTerrainMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastTerrainMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	lastTerrainMeshData->m_objectStatus = ObjectStatus::STANDBY;
}

void InnoAssetSystemNS::loadAssetsForComponents()
{
	for (auto l_environmentCaptureComponent : g_pCoreSystem->getGameSystem()->get<EnvironmentCaptureComponent>())
	{
		if (!l_environmentCaptureComponent->m_cubemapTextureFileName.empty())
		{
			//InnoAssetSystemNS::m_asyncTask.emplace_back(g_pCoreSystem->getTaskSystem()->submit([&]()
			//{
			//	l_environmentCaptureComponent->m_TDC = InnoAssetSystemNS::loadTexture(l_environmentCaptureComponent->m_cubemapTextureFileName, TextureUsageType::EQUIRETANGULAR);
			//}));
		}
	}

	for (auto l_visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (l_visibleComponent->m_visiblilityType != VisiblilityType::INNO_INVISIBLE)
		{
			if (l_visibleComponent->m_meshShapeType == MeshShapeType::CUSTOM)
			{
				if (!l_visibleComponent->m_modelFileName.empty())
				{
					InnoAssetSystemNS::m_asyncTask.emplace_back(g_pCoreSystem->getTaskSystem()->submit([&]()
					{
					}));
					l_visibleComponent->m_modelMap = InnoAssetSystemNS::loadModel(l_visibleComponent->m_modelFileName);
					l_visibleComponent->m_PhysicsDataComponent = g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(l_visibleComponent->m_modelMap, l_visibleComponent->m_parentEntity);
					l_visibleComponent->m_objectStatus = ObjectStatus::ALIVE;
				}
			}
			else
			{
				assignUnitMesh(l_visibleComponent->m_meshShapeType, l_visibleComponent);
				l_visibleComponent->m_PhysicsDataComponent = g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(l_visibleComponent->m_modelMap, l_visibleComponent->m_parentEntity);
				l_visibleComponent->m_objectStatus = ObjectStatus::ALIVE;
			}
		}
		g_pCoreSystem->getTaskSystem()->shrinkFutureContainer(InnoAssetSystemNS::m_asyncTask);
	}
}

void InnoAssetSystemNS::assignUnitMesh(MeshShapeType MeshUsageType, VisibleComponent* visibleComponent)
{
	MeshDataComponent* l_UnitMeshTemplate;
	switch (MeshUsageType)
	{
	case MeshShapeType::LINE: l_UnitMeshTemplate = m_UnitLineMDC; break;
	case MeshShapeType::QUAD: l_UnitMeshTemplate = m_UnitQuadMDC; break;
	case MeshShapeType::CUBE: l_UnitMeshTemplate = m_UnitCubeMDC; break;
	case MeshShapeType::SPHERE: l_UnitMeshTemplate = m_UnitSphereMDC; break;
	case MeshShapeType::TERRAIN: l_UnitMeshTemplate = m_TerrainMDC; break;
	case MeshShapeType::CUSTOM: g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: don't assign unit mesh to a custom mesh shape component!");
		break; return;
	}
	visibleComponent->m_modelMap.emplace(l_UnitMeshTemplate, addMaterialDataComponent());
}

ModelMap InnoAssetSystemNS::loadModel(const std::string & fileName)
{
	auto l_result = g_pCoreSystem->getFileSystem()->loadModel(fileName);
	return l_result;
}

TextureDataComponent* InnoAssetSystemNS::loadTexture(const std::string& fileName, TextureSamplerType textureSamplerType, TextureUsageType textureUsageType)
{
	auto l_TDC = g_pCoreSystem->getFileSystem()->loadTexture(fileName);
	l_TDC->m_textureDataDesc.textureSamplerType = textureSamplerType;
	l_TDC->m_textureDataDesc.textureUsageType = textureUsageType;
	return l_TDC;
}