#include "AssetSystem.h"

#include "../common/ComponentHeaders.h"

#include "../component/AssetSystemComponent.h"
#include "../component/GameSystemComponent.h"

namespace fs = std::filesystem;

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoAssetSystemNS
{
	void loadFolderData();
	void loadDefaultAssets();
	void loadAssetsForComponents();

	ModelMap loadModel(const std::string& fileName);
	TextureDataComponent* loadTexture(const std::string& fileName, TextureUsageType textureUsageType);

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

	std::vector<InnoFuture<void>> m_asyncTaskVector;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::setup()
{
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
	auto newMesh = g_pCoreSystem->getMemorySystem()->spawn<MeshDataComponent>();
	auto l_parentEntity = InnoMath::createEntityID();
	newMesh->m_parentEntity = l_parentEntity;
	auto l_meshMap = &AssetSystemComponent::get().m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, MeshDataComponent*>(l_parentEntity, newMesh));
	return newMesh;
}

MaterialDataComponent* InnoAssetSystemNS::addMaterialDataComponent()
{
	auto newMaterial = g_pCoreSystem->getMemorySystem()->spawn<MaterialDataComponent>();
	auto l_parentEntity = InnoMath::createEntityID();
	newMaterial->m_parentEntity = l_parentEntity;
	auto l_materialMap = &AssetSystemComponent::get().m_materialMap;
	l_materialMap->emplace(std::pair<EntityID, MaterialDataComponent*>(l_parentEntity, newMaterial));
	return newMaterial;
}

TextureDataComponent* InnoAssetSystemNS::addTextureDataComponent()
{
	auto newTexture = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();
	auto l_parentEntity = InnoMath::createEntityID();
	newTexture->m_parentEntity = l_parentEntity;
	auto l_textureMap = &AssetSystemComponent::get().m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, TextureDataComponent*>(l_parentEntity, newTexture));
	return newTexture;
}

MeshDataComponent* InnoAssetSystem::getMeshDataComponent(EntityID EntityID)
{
	auto result = AssetSystemComponent::get().m_meshMap.find(EntityID);
	if (result != AssetSystemComponent::get().m_meshMap.end())
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
	auto result = AssetSystemComponent::get().m_textureMap.find(EntityID);
	if (result != AssetSystemComponent::get().m_textureMap.end())
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
		return AssetSystemComponent::get().m_UnitLineTemplate; break;
	case MeshShapeType::QUAD:
		return AssetSystemComponent::get().m_UnitQuadTemplate; break;
	case MeshShapeType::CUBE:
		return AssetSystemComponent::get().m_UnitCubeTemplate; break;
	case MeshShapeType::SPHERE:
		return AssetSystemComponent::get().m_UnitSphereTemplate; break;
	case MeshShapeType::TERRAIN:
		return AssetSystemComponent::get().m_Terrain; break;
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
		return AssetSystemComponent::get().m_basicNormalTemplate; break;
	case TextureUsageType::ALBEDO:
		return AssetSystemComponent::get().m_basicAlbedoTemplate; break;
	case TextureUsageType::METALLIC:
		return AssetSystemComponent::get().m_basicMetallicTemplate; break;
	case TextureUsageType::ROUGHNESS:
		return AssetSystemComponent::get().m_basicRoughnessTemplate; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return AssetSystemComponent::get().m_basicAOTemplate; break;
	case TextureUsageType::CUBEMAP:
		return nullptr; break;
	case TextureUsageType::EQUIRETANGULAR:
		return nullptr; break;
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
		return AssetSystemComponent::get().m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return AssetSystemComponent::get().m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return AssetSystemComponent::get().m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return AssetSystemComponent::get().m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return AssetSystemComponent::get().m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return AssetSystemComponent::get().m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return AssetSystemComponent::get().m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

bool InnoAssetSystem::removeMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &AssetSystemComponent::get().m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		g_pCoreSystem->getMemorySystem()->destroy<MeshDataComponent>(l_mesh->second);
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
	auto l_textureMap = &AssetSystemComponent::get().m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			// @TODO
		}

		g_pCoreSystem->getMemorySystem()->destroy<TextureDataComponent>(l_texture->second);
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
	auto l_meshMap = &AssetSystemComponent::get().m_meshMap;
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
	auto l_textureMap = &AssetSystemComponent::get().m_textureMap;
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

	//meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	//for (auto& l_vertexData : meshDataComponent.m_vertices)
	//{
	//	l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	//}

	meshDataComponent.m_indices.reserve(36);

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

	f_directoryTreeBuilder(path, 0, &AssetSystemComponent::get().m_rootDirectoryMetadata);
	f_assignParentDirectory(&AssetSystemComponent::get().m_rootDirectoryMetadata);
}

void InnoAssetSystemNS::loadDefaultAssets()
{
	InnoAssetSystemNS::loadFolderData();

	AssetSystemComponent::get().m_basicNormalTemplate = loadTexture("..//res//textures//basic_normal.png", TextureUsageType::NORMAL);
	AssetSystemComponent::get().m_basicAlbedoTemplate = loadTexture("..//res//textures//basic_albedo.png", TextureUsageType::ALBEDO);
	AssetSystemComponent::get().m_basicMetallicTemplate = loadTexture("..//res//textures//basic_metallic.png", TextureUsageType::METALLIC);
	AssetSystemComponent::get().m_basicRoughnessTemplate = loadTexture("..//res//textures//basic_roughness.png", TextureUsageType::ROUGHNESS);
	AssetSystemComponent::get().m_basicAOTemplate = loadTexture("..//res//textures//basic_ao.png", TextureUsageType::AMBIENT_OCCLUSION);

	AssetSystemComponent::get().m_iconTemplate_OBJ = loadTexture("..//res//textures//InnoFileTypeIcons_OBJ.png", TextureUsageType::NORMAL);
	AssetSystemComponent::get().m_iconTemplate_PNG = loadTexture("..//res//textures//InnoFileTypeIcons_PNG.png", TextureUsageType::NORMAL);
	AssetSystemComponent::get().m_iconTemplate_SHADER = loadTexture("..//res//textures//InnoFileTypeIcons_SHADER.png", TextureUsageType::NORMAL);
	AssetSystemComponent::get().m_iconTemplate_UNKNOWN = loadTexture("..//res//textures//InnoFileTypeIcons_UNKNOWN.png", TextureUsageType::NORMAL);

	AssetSystemComponent::get().m_iconTemplate_DirectionalLight = loadTexture("..//res//textures//InnoWorldEditorIcons_DirectionalLight.png", TextureUsageType::NORMAL);
	AssetSystemComponent::get().m_iconTemplate_PointLight = loadTexture("..//res//textures//InnoWorldEditorIcons_PointLight.png", TextureUsageType::NORMAL);
	AssetSystemComponent::get().m_iconTemplate_SphereLight = loadTexture("..//res//textures//InnoWorldEditorIcons_SphereLight.png", TextureUsageType::NORMAL);

	AssetSystemComponent::get().m_UnitLineTemplate = addMeshDataComponent();
	auto lastLineMeshData = AssetSystemComponent::get().m_UnitLineTemplate;
	InnoAssetSystemNS::addUnitLine(*lastLineMeshData);
	lastLineMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastLineMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	lastLineMeshData->m_objectStatus = ObjectStatus::STANDBY;

	AssetSystemComponent::get().m_UnitQuadTemplate = addMeshDataComponent();
	auto lastQuadMeshData = AssetSystemComponent::get().m_UnitQuadTemplate;
	InnoAssetSystemNS::addUnitQuad(*lastQuadMeshData);
	lastQuadMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastQuadMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	lastQuadMeshData->m_objectStatus = ObjectStatus::STANDBY;

	AssetSystemComponent::get().m_UnitCubeTemplate = addMeshDataComponent();
	auto lastCubeMeshData = AssetSystemComponent::get().m_UnitCubeTemplate;
	InnoAssetSystemNS::addUnitCube(*lastCubeMeshData);
	lastCubeMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastCubeMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	lastCubeMeshData->m_objectStatus = ObjectStatus::STANDBY;

	AssetSystemComponent::get().m_UnitSphereTemplate = addMeshDataComponent();
	auto lastSphereMeshData = AssetSystemComponent::get().m_UnitSphereTemplate;
	InnoAssetSystemNS::addUnitSphere(*lastSphereMeshData);
	lastSphereMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastSphereMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	lastSphereMeshData->m_objectStatus = ObjectStatus::STANDBY;

	AssetSystemComponent::get().m_Terrain = addMeshDataComponent();
	auto lastTerrainMeshData = AssetSystemComponent::get().m_Terrain;
	InnoAssetSystemNS::addTerrain(*lastTerrainMeshData);
	lastTerrainMeshData->m_meshUsageType = MeshUsageType::STATIC;
	lastTerrainMeshData->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	lastTerrainMeshData->m_objectStatus = ObjectStatus::STANDBY;
}

void InnoAssetSystemNS::loadAssetsForComponents()
{
	for (auto& l_environmentCaptureComponent : GameSystemComponent::get().m_EnvironmentCaptureComponents)
	{
		if (!l_environmentCaptureComponent->m_cubemapTextureFileName.empty())
		{
			//l_environmentCaptureComponent->m_TDC = InnoAssetSystemNS::loadTexture(l_environmentCaptureComponent->m_cubemapTextureFileName, TextureUsageType::EQUIRETANGULAR);
		}
	}
	for (auto& l_visibleComponent : GameSystemComponent::get().m_VisibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType != VisiblilityType::INNO_INVISIBLE)
		{
			if (l_visibleComponent->m_meshShapeType == MeshShapeType::CUSTOM)
			{
				if (l_visibleComponent->m_modelFileName != "")
				{	
					InnoAssetSystemNS::m_asyncTaskVector.emplace_back(g_pCoreSystem->getTaskSystem()->submit([&]()
					{
						l_visibleComponent->m_modelMap = InnoAssetSystemNS::loadModel(l_visibleComponent->m_modelFileName);
						l_visibleComponent->m_objectStatus = ObjectStatus::STANDBY;
						g_pCoreSystem->getPhysicsSystem()->generatePhysicsData(l_visibleComponent);
					}));
				}
			}
			else
			{
				assignUnitMesh(l_visibleComponent->m_meshShapeType, l_visibleComponent);
				g_pCoreSystem->getPhysicsSystem()->generatePhysicsData(l_visibleComponent);
			}
		}
	}
}

void InnoAssetSystemNS::assignUnitMesh(MeshShapeType MeshUsageType, VisibleComponent* visibleComponent)
{
	MeshDataComponent* l_UnitMeshTemplate;
	switch (MeshUsageType)
	{
	case MeshShapeType::LINE: l_UnitMeshTemplate = AssetSystemComponent::get().m_UnitLineTemplate; break;
	case MeshShapeType::QUAD: l_UnitMeshTemplate = AssetSystemComponent::get().m_UnitQuadTemplate; break;
	case MeshShapeType::CUBE: l_UnitMeshTemplate = AssetSystemComponent::get().m_UnitCubeTemplate; break;
	case MeshShapeType::SPHERE: l_UnitMeshTemplate = AssetSystemComponent::get().m_UnitSphereTemplate; break;
	case MeshShapeType::TERRAIN: l_UnitMeshTemplate = AssetSystemComponent::get().m_Terrain; break;
	case MeshShapeType::CUSTOM: break;
	}
	visibleComponent->m_modelMap.emplace(l_UnitMeshTemplate, addMaterialDataComponent());
	visibleComponent->m_objectStatus = ObjectStatus::STANDBY;
}

ModelMap InnoAssetSystemNS::loadModel(const std::string & fileName)
{
	auto l_result = g_pCoreSystem->getFileSystem()->loadModel(fileName);
	return l_result;
}

TextureDataComponent* InnoAssetSystemNS::loadTexture(const std::string& fileName, TextureUsageType textureUsageType)
{
	auto l_TDC = g_pCoreSystem->getFileSystem()->loadTexture(fileName);
	l_TDC->m_textureDataDesc.textureUsageType = textureUsageType;
	return l_TDC;
}