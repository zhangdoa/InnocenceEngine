#include "AssetSystem.h"
#include "../Common/ComponentHeaders.h"
#include "../Common/InnoMathHelper.h"
#include "../Core/InnoLogger.h"

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#define recordLoaded( funcName, type, value ) \
bool InnoAssetSystem::recordLoaded##funcName(const char * fileName, type value) \
{ \
	m_loaded##funcName.emplace(fileName, value); \
\
	return true; \
}

#define findLoaded( funcName, type, value ) \
bool InnoAssetSystem::findLoaded##funcName(const char * fileName, type value) \
{ \
	auto l_loaded##funcName = m_loaded##funcName.find(fileName); \
	if (l_loaded##funcName != m_loaded##funcName.end()) \
	{ \
		value = l_loaded##funcName->second; \
	\
		return true; \
	} \
	else \
	{ \
	InnoLogger::Log(LogLevel::Verbose, "AssetSystem: ", fileName, " has not been loaded."); \
	\
	return false; \
	} \
}

namespace InnoAssetSystemNS
{
	std::vector<MeshMaterialPair> m_meshMaterialPairPool;
	std::unordered_map<std::string, MeshMaterialPair> m_loadedMeshMaterialPair;
	std::unordered_map<std::string, ModelIndex> m_loadedModel;
	std::unordered_map<std::string, TextureDataComponent*> m_loadedTexture;
	std::unordered_map<std::string, AnimationDataComponent*> m_loadedAnimation;
	std::unordered_map<std::string, SkeletonDataComponent*> m_loadedSkeleton;

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

using namespace InnoAssetSystemNS;

bool InnoAssetSystem::setup()
{
	//@TODO:
	m_meshMaterialPairPool.reserve(65536);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoAssetSystem::initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "AssetSystem has been initialized.");
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "AssetSystem: Object is not created!");
		return false;
	}
}

bool InnoAssetSystem::update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoAssetSystem::terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "AssetSystem has been terminated.");
	return true;
}

ObjectStatus InnoAssetSystem::getStatus()
{
	return m_ObjectStatus;
}

recordLoaded(MeshMaterialPair, const MeshMaterialPair&, pair)
findLoaded(MeshMaterialPair, MeshMaterialPair&, pair)

recordLoaded(Model, const ModelIndex&, modelIndex)
findLoaded(Model, ModelIndex&, modelIndex)

recordLoaded(Texture, TextureDataComponent*, texture)
findLoaded(Texture, TextureDataComponent*&, texture)

recordLoaded(Skeleton, SkeletonDataComponent*, skeleton)
findLoaded(Skeleton, SkeletonDataComponent*&, skeleton)

recordLoaded(Animation, AnimationDataComponent*, animation)
findLoaded(Animation, AnimationDataComponent*&, animation)

uint64_t InnoAssetSystem::getCurrentMeshMaterialPairOffset()
{
	return m_meshMaterialPairPool.size();
}

uint64_t InnoAssetSystem::addMeshMaterialPair(const MeshMaterialPair & pair)
{
	m_meshMaterialPairPool.emplace_back(pair);
	return m_meshMaterialPairPool.size();
}

const MeshMaterialPair & InnoAssetSystem::getMeshMaterialPair(uint64_t index)
{
	return m_meshMaterialPairPool[index];
}

ModelIndex InnoAssetSystem::addUnitModel(MeshShapeType meshShapeType)
{
	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(meshShapeType);
	auto l_material = g_pModuleManager->getRenderingFrontend()->addMaterialDataComponent();
	l_material->m_ObjectStatus = ObjectStatus::Created;

	MeshMaterialPair l_pair;
	l_pair.mesh = l_mesh;
	l_pair.material = l_material;

	ModelIndex l_result;
	l_result.m_startOffset = getCurrentMeshMaterialPairOffset();
	l_result.m_count = 1;

	addMeshMaterialPair(l_pair);

	return l_result;
}

void InnoAssetSystem::addUnitCube(MeshDataComponent& meshDataComponent)
{
	meshDataComponent.m_vertices.reserve(8);
	meshDataComponent.m_vertices.fulfill();

	InnoMath::generateNDC(&meshDataComponent.m_vertices[0]);

	std::vector<Index> l_indices =
	{
		0, 3, 1, 1, 3, 2,
		4, 0, 5, 5, 0, 1,
		7, 4, 6, 6, 4, 5,
		3, 7, 2, 2, 7, 6,
		7, 0, 4, 0, 7, 3,
		1, 2, 5, 5, 2, 6
	};

	meshDataComponent.m_indices.reserve(36);
	meshDataComponent.m_indices.fulfill();

	for (uint32_t i = 0; i < 36; i++)
	{
		meshDataComponent.m_indices[i] = l_indices[i];
	}

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addUnitSphere(MeshDataComponent& meshDataComponent)
{
	auto radius = 1.0f;
	auto sectorCount = 64;
	auto stackCount = 64;

	float x, y, z, xy;                              // vertex position
	float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
	float s, t;                                     // vertex texCoord

	float sectorStep = 2 * PI<float> / sectorCount;
	float stackStep = PI<float> / stackCount;
	float sectorAngle, stackAngle;

	meshDataComponent.m_vertices.reserve((stackCount + 1) * (sectorCount + 1));

	for (int32_t i = 0; i <= stackCount; ++i)
	{
		stackAngle = PI<float> / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = radius * cosf(stackAngle);             // r * cos(u)
		z = radius * sinf(stackAngle);              // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// the first and last vertices have same position and normal, but different tex coords
		for (int32_t j = 0; j <= sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			Vertex l_VertexData;
			l_VertexData.m_pos = Vec4(x, y, z, 1.0f);

			// normalized vertex normal (nx, ny, nz)
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			l_VertexData.m_normal = Vec4(nx, ny, nz, 1.0f);

			// vertex tex coord (s, t) range between [0, 1]
			s = (float)j / sectorCount;
			t = (float)i / stackCount;
			l_VertexData.m_texCoord = Vec2(s, t);

			meshDataComponent.m_vertices.emplace_back(l_VertexData);
		}
	}

	meshDataComponent.m_indices.reserve(stackCount * (sectorCount - 1) * 6);

	int32_t k1, k2;
	for (int32_t i = 0; i < stackCount; ++i)
	{
		k1 = i * (sectorCount + 1);     // beginning of current stack
		k2 = k1 + sectorCount + 1;      // beginning of next stack

		for (int32_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				meshDataComponent.m_indices.emplace_back(k1);
				meshDataComponent.m_indices.emplace_back(k2);
				meshDataComponent.m_indices.emplace_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				meshDataComponent.m_indices.emplace_back(k1 + 1);
				meshDataComponent.m_indices.emplace_back(k2);
				meshDataComponent.m_indices.emplace_back(k2 + 1);
			}
		}
	}

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addUnitQuad(MeshDataComponent& meshDataComponent)
{
	meshDataComponent.m_vertices.reserve(4);
	meshDataComponent.m_vertices.fulfill();

	meshDataComponent.m_vertices[0].m_pos = Vec4(1.0f, 1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[0].m_texCoord = Vec2(1.0f, 1.0f);

	meshDataComponent.m_vertices[1].m_pos = Vec4(1.0f, -1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[1].m_texCoord = Vec2(1.0f, 0.0f);

	meshDataComponent.m_vertices[2].m_pos = Vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[2].m_texCoord = Vec2(0.0f, 0.0f);

	meshDataComponent.m_vertices[3].m_pos = Vec4(-1.0f, 1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[3].m_texCoord = Vec2(0.0f, 1.0f);

	meshDataComponent.m_indices.reserve(6);
	meshDataComponent.m_indices.fulfill();

	meshDataComponent.m_indices[0] = 0;
	meshDataComponent.m_indices[1] = 1;
	meshDataComponent.m_indices[2] = 3;
	meshDataComponent.m_indices[3] = 1;
	meshDataComponent.m_indices[4] = 2;
	meshDataComponent.m_indices[5] = 3;

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addUnitLine(MeshDataComponent& meshDataComponent)
{
	meshDataComponent.m_vertices.reserve(2);
	meshDataComponent.m_vertices.fulfill();

	meshDataComponent.m_vertices[0].m_pos = Vec4(1.0f, 1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[0].m_texCoord = Vec2(1.0f, 1.0f);

	meshDataComponent.m_vertices[1].m_pos = Vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[1].m_texCoord = Vec2(0.0f, 0.0f);

	meshDataComponent.m_indices.reserve(2);
	meshDataComponent.m_indices.fulfill();

	meshDataComponent.m_indices[0] = 0;
	meshDataComponent.m_indices[1] = 1;

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addTerrain(MeshDataComponent& meshDataComponent)
{
	auto l_gridSize = 1024;
	auto l_gridSize2 = l_gridSize * l_gridSize;
	auto l_gridSizehalf = l_gridSize / 2;
	meshDataComponent.m_vertices.reserve(l_gridSize2 * 4);
	meshDataComponent.m_indices.reserve(l_gridSize2 * 6);

	for (auto j = 0; j < l_gridSize; j++)
	{
		for (auto i = 0; i < l_gridSize; i++)
		{
			auto l_px0 = (float)(i - l_gridSizehalf) * 100.0f;
			auto l_px1 = l_px0 + 100.0f;
			auto l_pz0 = (float)(j - l_gridSizehalf) * 100.0f;
			auto l_pz1 = l_pz0 + 100.0f;

			auto l_tx0 = l_px0 / (float)l_gridSize;
			auto l_tx1 = l_px1 / (float)l_gridSize;
			auto l_tz0 = l_pz0 / (float)l_gridSize;
			auto l_tz1 = l_pz1 / (float)l_gridSize;

			Vertex l_VertexData_1;
			l_VertexData_1.m_pos = Vec4(l_px0, 0.0f, l_pz0, 1.0f);
			l_VertexData_1.m_texCoord = Vec2(l_tx0, l_tz0);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_1);

			Vertex l_VertexData_2;
			l_VertexData_2.m_pos = Vec4(l_px0, 0.0f, l_pz1, 1.0f);
			l_VertexData_2.m_texCoord = Vec2(l_tx0, l_tz1);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_2);

			Vertex l_VertexData_3;
			l_VertexData_3.m_pos = Vec4(l_px1, 0.0f, l_pz1, 1.0f);
			l_VertexData_3.m_texCoord = Vec2(l_tx1, l_tz1);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_3);

			Vertex l_VertexData_4;
			l_VertexData_4.m_pos = Vec4(l_px1, 0.0f, l_pz0, 1.0f);
			l_VertexData_4.m_texCoord = Vec2(l_tx1, l_tz0);
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