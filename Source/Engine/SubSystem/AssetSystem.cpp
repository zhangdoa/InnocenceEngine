#include "AssetSystem.h"
#include "../Common/ComponentHeaders.h"
#include "../Common/CommonMacro.inl"
#include "../Common/InnoMathHelper.h"
#include "../Core/InnoLogger.h"
#include "../Core/IOService.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"
#include "../ThirdParty/STBWrapper/STBWrapper.h"
#include "../ThirdParty/AssimpWrapper/AssimpWrapper.h"

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

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
	void addTriangle(MeshDataComponent* meshDataComponent);
	void addSquare(MeshDataComponent* meshDataComponent);
	void addPentagon(MeshDataComponent* meshDataComponent);
	void addHexagon(MeshDataComponent* meshDataComponent);
	void addTetrahedron(MeshDataComponent* meshDataComponent);
	void addCube(MeshDataComponent* meshDataComponent);
	void addOctahedron(MeshDataComponent* meshDataComponent);
	void addDodecahedron(MeshDataComponent* meshDataComponent);
	void addIcosahedron(MeshDataComponent* meshDataComponent);
	void addSphere(MeshDataComponent* meshDataComponent);
	void addTerrain(MeshDataComponent* meshDataComponent);

	std::function<void(VisibleComponent*, bool)> f_LoadModelTask;
	std::function<void(VisibleComponent*, bool)> f_AssignProceduralModelTask;
	std::function<void(VisibleComponent*)> f_GeneratePDCTask;

	TObjectPool<MeshMaterialPair>* m_meshMaterialPairPool;
	TObjectPool<Model>* m_modelPool;
	std::vector<MeshMaterialPair*> m_meshMaterialPairList;

	std::unordered_map<std::string, MeshMaterialPair*> m_loadedMeshMaterialPair;
	std::unordered_map<std::string, Model*> m_loadedModel;
	std::unordered_map<std::string, TextureDataComponent*> m_loadedTexture;
	std::unordered_map<std::string, AnimationDataComponent*> m_loadedAnimation;
	std::unordered_map<std::string, SkeletonDataComponent*> m_loadedSkeleton;

	std::atomic<uint64_t> m_currentMeshMaterialPairIndex = 0;
	std::shared_mutex m_mutexMeshMaterialPair;
	std::shared_mutex m_mutexModel;

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

using namespace InnoAssetSystemNS;

void generateVerticesForPolygon(MeshDataComponent* meshDataComponent, uint32_t sectorCount)
{
	meshDataComponent->m_vertices.reserve(sectorCount);
	meshDataComponent->m_vertices.fulfill();

	auto l_sectorCount = (float)sectorCount;
	auto l_sectorStep = 2.0f * PI<float> / l_sectorCount;

	for (size_t i = 0; i < sectorCount; i++)
	{
		auto l_pos = Vec4(-sinf(l_sectorStep * (float)i), cosf(l_sectorStep * (float)i), 0.0f, 1.0f);
		meshDataComponent->m_vertices[i].m_pos = l_pos;
		meshDataComponent->m_vertices[i].m_texCoord = Vec2(l_pos.x, l_pos.y) * 0.5f + 0.5f;
	}
}

void generateIndicesForPolygon(MeshDataComponent* meshDataComponent, uint32_t sectorCount)
{
	meshDataComponent->m_indices.reserve((sectorCount - 2) * 3);
	meshDataComponent->m_indices.fulfill();
	meshDataComponent->m_indicesSize = meshDataComponent->m_indices.capacity();

	uint32_t l_currentIndex = 0;

	for (size_t i = 0; i < meshDataComponent->m_indicesSize; i += 3)
	{
		meshDataComponent->m_indices[i] = l_currentIndex;
		meshDataComponent->m_indices[i + 1] = l_currentIndex + 1;
		meshDataComponent->m_indices[i + 2] = sectorCount - 1;
		l_currentIndex++;
	}
}

void InnoAssetSystemNS::addTriangle(MeshDataComponent* meshDataComponent)
{
	generateVerticesForPolygon(meshDataComponent, 3);
	generateIndicesForPolygon(meshDataComponent, 3);
}

void InnoAssetSystemNS::addSquare(MeshDataComponent* meshDataComponent)
{
	meshDataComponent->m_vertices.reserve(4);
	meshDataComponent->m_vertices.fulfill();

	meshDataComponent->m_vertices[0].m_pos = Vec4(1.0f, 1.0f, 0.0f, 1.0f);
	meshDataComponent->m_vertices[0].m_texCoord = Vec2(1.0f, 1.0f);

	meshDataComponent->m_vertices[1].m_pos = Vec4(1.0f, -1.0f, 0.0f, 1.0f);
	meshDataComponent->m_vertices[1].m_texCoord = Vec2(1.0f, 0.0f);

	meshDataComponent->m_vertices[2].m_pos = Vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	meshDataComponent->m_vertices[2].m_texCoord = Vec2(0.0f, 0.0f);

	meshDataComponent->m_vertices[3].m_pos = Vec4(-1.0f, 1.0f, 0.0f, 1.0f);
	meshDataComponent->m_vertices[3].m_texCoord = Vec2(0.0f, 1.0f);

	meshDataComponent->m_indices.reserve(6);
	meshDataComponent->m_indices.fulfill();

	meshDataComponent->m_indices[0] = 0;
	meshDataComponent->m_indices[1] = 1;
	meshDataComponent->m_indices[2] = 3;
	meshDataComponent->m_indices[3] = 1;
	meshDataComponent->m_indices[4] = 2;
	meshDataComponent->m_indices[5] = 3;

	meshDataComponent->m_indicesSize = meshDataComponent->m_indices.size();
}

void InnoAssetSystemNS::addPentagon(MeshDataComponent* meshDataComponent)
{
	generateVerticesForPolygon(meshDataComponent, 5);
	generateIndicesForPolygon(meshDataComponent, 5);
}

void InnoAssetSystemNS::addHexagon(MeshDataComponent* meshDataComponent)
{
	generateVerticesForPolygon(meshDataComponent, 6);
	generateIndicesForPolygon(meshDataComponent, 6);
}

void generateVertexBasedNormal(MeshDataComponent* meshDataComponent)
{
	auto l_verticesCount = meshDataComponent->m_vertices.size();
	for (size_t i = 0; i < l_verticesCount; i++)
	{
		meshDataComponent->m_vertices[i].m_normal = meshDataComponent->m_vertices[i].m_pos;
		meshDataComponent->m_vertices[i].m_normal.w = 0.0f;
		meshDataComponent->m_vertices[i].m_normal = meshDataComponent->m_vertices[i].m_normal.normalize();
	}
}

void generateFaceBasedNormal(MeshDataComponent* meshDataComponent, uint32_t verticesPerFace)
{
	auto l_face = meshDataComponent->m_indices.size() / verticesPerFace;

	for (size_t i = 0; i < l_face; i++)
	{
		auto l_normal = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

		for (size_t j = 0; j < verticesPerFace; j++)
		{
			l_normal = l_normal + meshDataComponent->m_vertices[i * verticesPerFace + j].m_pos;
		}
		l_normal = l_normal / (float)verticesPerFace;
		l_normal.w = 0.0f;
		l_normal = l_normal.normalize();

		for (size_t j = 0; j < verticesPerFace; j++)
		{
			meshDataComponent->m_vertices[i * verticesPerFace + j].m_normal = l_normal;
		}
	}
}

void fulfillVerticesAndIndicesForPolyhedron(MeshDataComponent* meshDataComponent, const std::vector<Index>& indices, const std::vector<Vec4>& vertices, uint32_t verticesPerFace = 0)
{
	meshDataComponent->m_vertices.reserve(indices.size());
	meshDataComponent->m_vertices.fulfill();

	for (uint32_t i = 0; i < indices.size(); i++)
	{
		meshDataComponent->m_vertices[i].m_pos = vertices[indices[i]];
	}

	meshDataComponent->m_indices.reserve(indices.size());
	meshDataComponent->m_indices.fulfill();

	for (uint32_t i = 0; i < indices.size(); i++)
	{
		meshDataComponent->m_indices[i] = i;
	}

	meshDataComponent->m_indicesSize = meshDataComponent->m_indices.size();

	if (verticesPerFace)
	{
		generateFaceBasedNormal(meshDataComponent, verticesPerFace);
	}
	else
	{
		generateVertexBasedNormal(meshDataComponent);
	}
}

void InnoAssetSystemNS::addTetrahedron(MeshDataComponent* meshDataComponent)
{
	std::vector<Index> l_indices =
	{
		0, 3, 1, 0, 2, 3,
		0, 1, 2, 1, 3, 2
	};

	std::vector<Vec4> l_vertices =
	{
		Vec4(1.0f, 1.0f, 1.0f, 1.0f),
		Vec4(1.0f, -1.0f, -1.0f, 1.0f),
		Vec4(-1.0f, 1.0f, -1.0f, 1.0f),
		Vec4(-1.0f, -1.0f, 1.0f, 1.0f)
	};

	fulfillVerticesAndIndicesForPolyhedron(meshDataComponent, l_indices, l_vertices, 3);
}

void InnoAssetSystemNS::addCube(MeshDataComponent* meshDataComponent)
{
	std::vector<Index> l_indices =
	{
		0, 3, 1, 1, 3, 2,
		4, 0, 5, 5, 0, 1,
		7, 4, 6, 6, 4, 5,
		3, 7, 2, 2, 7, 6,
		4, 7, 0, 0, 7, 3,
		1, 2, 5, 5, 2, 6
	};

	std::vector<Vec4> l_vertices =
	{
		Vec4(1.0f, 1.0f, 1.0f, 1.0f),
		Vec4(1.0f, -1.0f, 1.0f, 1.0f),
		Vec4(-1.0f, -1.0f, 1.0f, 1.0f),
		Vec4(-1.0f, 1.0f, 1.0f, 1.0f),
		Vec4(1.0f, 1.0f, -1.0f, 1.0f),
		Vec4(1.0f, -1.0f, -1.0f, 1.0f),
		Vec4(-1.0f, -1.0f, -1.0f, 1.0f),
		Vec4(-1.0f, 1.0f, -1.0f, 1.0f)
	};

	fulfillVerticesAndIndicesForPolyhedron(meshDataComponent, l_indices, l_vertices, 6);
}

void InnoAssetSystemNS::addOctahedron(MeshDataComponent* meshDataComponent)
{
	std::vector<Index> l_indices =
	{
		0, 2, 4, 4, 2, 1,
		1, 2, 5, 5, 2, 0,
		0, 4, 3, 4, 1, 3,
		1, 5, 3, 5, 0, 3
	};

	std::vector<Vec4> l_vertices =
	{
		Vec4(1.0f, 0.0f, 0.0f, 1.0f),
		Vec4(-1.0f, 0.0f, 0.0f, 1.0f),
		Vec4(0.0f, 1.0f, 0.0f, 1.0f),
		Vec4(0.0f, -1.0f, 0.0f, 1.0f),
		Vec4(0.0f, 0.0f, 1.0f, 1.0f),
		Vec4(0.0f, 0.0f, -1.0f, 1.0f)
	};

	fulfillVerticesAndIndicesForPolyhedron(meshDataComponent, l_indices, l_vertices, 3);
}

void InnoAssetSystemNS::addDodecahedron(MeshDataComponent* meshDataComponent)
{
	std::vector<Index> l_indices =
	{
		0, 1, 4, 1, 2, 4, 2, 3, 4,
		5, 6, 9, 6, 7, 9, 7, 8, 9,
		10, 11, 12, 11, 3, 12, 3, 2, 12,
		13, 14, 15, 14, 8, 15, 8, 7, 15,

		3, 11, 4, 11, 16, 4, 16, 17, 4,
		2, 1, 12, 1, 18, 12, 18, 19, 12,
		7, 6, 15, 6, 17, 15, 17, 16, 15,
		8, 14, 9, 14, 19, 9, 19, 18, 9,

		17, 6, 4, 6, 5, 4, 5, 0, 4,
		16, 11, 15, 11, 10, 15, 10, 13, 15,
		18, 1, 9, 1, 0, 9, 0, 5, 9,
		19, 14, 12, 14, 13, 12, 13, 10, 12
	};

	std::vector<Vec4> l_vertices =
	{
		Vec4(0.0f, 1.61803398875f, 0.61803398875f, 1.0f),
		Vec4(-1.0f, 1.0f, 1.0f, 1.0f),
		Vec4(-0.61803398875f, 0.0f, 1.61803398875f, 1.0f),
		Vec4(0.61803398875f, 0.0f, 1.61803398875f, 1.0f),
		Vec4(1.0f, 1.0f, 1.0f, 1.0f),
		Vec4(0.0f, 1.61803398875f, -0.61803398875f, 1.0f),
		Vec4(1.0f, 1.0f, -1.0f, 1.0f),
		Vec4(0.61803398875f, 0.0f, -1.61803398875f, 1.0f),
		Vec4(-0.61803398875f, 0.0f, -1.61803398875f, 1.0f),
		Vec4(-1.0f, 1.0f, -1.0f, 1.0f),
		Vec4(0.0f, -1.61803398875f, 0.61803398875f, 1.0f),
		Vec4(1.0f, -1.0f, 1.0f, 1.0f),
		Vec4(-1.0f, -1.0f, 1.0f, 1.0f),
		Vec4(0.0f, -1.61803398875f, -0.61803398875f, 1.0f),
		Vec4(-1.0f, -1.0f, -1.0f, 1.0f),
		Vec4(1.0f, -1.0f, -1.0f, 1.0f),
		Vec4(1.61803398875f, -0.61803398875f, 0.0f, 1.0f),
		Vec4(1.61803398875f, 0.61803398875f, 0.0f, 1.0f),
		Vec4(-1.61803398875f, 0.61803398875f, 0.0f, 1.0f),
		Vec4(-1.61803398875f, -0.61803398875f, 0.0f, 1.0f)
	};

	fulfillVerticesAndIndicesForPolyhedron(meshDataComponent, l_indices, l_vertices, 9);
}

void InnoAssetSystemNS::addIcosahedron(MeshDataComponent* meshDataComponent)
{
	std::vector<Index> l_indices =
	{
		0, 1, 2, 0, 2, 3,
		0, 3, 4, 0, 4, 5,
		0, 5, 1, 1, 8, 2,
		2, 7, 3, 3, 6, 4,
		4, 10, 5, 5, 9, 1,
		1, 9, 8, 2, 8, 7,
		3, 7, 6, 4, 6, 10,
		5, 10, 9, 11, 9, 10,
		11, 8, 9, 11, 7, 8,
		11, 6, 7, 11, 10, 6
	};

	std::vector<Vec4> l_vertices =
	{
		Vec4(1.0f, 0.0f, 0.0f, 1.0f),
		Vec4(0.447213595500f, 0.894427191000f, 0.0f, 1.0f),
		Vec4(0.447213595500f, 0.276393202252f, 0.850650808354f, 1.0f),
		Vec4(0.447213595500f, -0.723606797748f, 0.525731112119f, 1.0f),
		Vec4(0.447213595500f, -0.723606797748f, -0.525731112119f, 1.0f),
		Vec4(0.447213595500f, 0.276393202252f, -0.850650808354f, 1.0f),
		Vec4(-0.447213595500f, -0.894427191000f, 0.0f, 1.0f),
		Vec4(-0.447213595500f, -0.276393202252f, 0.850650808354f, 1.0f),
		Vec4(-0.447213595500f, 0.723606797748f, 0.525731112119f, 1.0f),
		Vec4(-0.447213595500f, 0.723606797748f, -0.525731112119f, 1.0f),
		Vec4(-0.447213595500f, -0.276393202252f, -0.850650808354f, 1.0f),
		Vec4(-1.0f, 0.0f, 0.0f, 1.0f)
	};

	fulfillVerticesAndIndicesForPolyhedron(meshDataComponent, l_indices, l_vertices, 3);
}

void InnoAssetSystemNS::addSphere(MeshDataComponent* meshDataComponent)
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

	meshDataComponent->m_vertices.reserve((stackCount + 1) * (sectorCount + 1));

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

			meshDataComponent->m_vertices.emplace_back(l_VertexData);
		}
	}

	meshDataComponent->m_indices.reserve(stackCount * (sectorCount - 1) * 6);

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
				meshDataComponent->m_indices.emplace_back(k1);
				meshDataComponent->m_indices.emplace_back(k2);
				meshDataComponent->m_indices.emplace_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				meshDataComponent->m_indices.emplace_back(k1 + 1);
				meshDataComponent->m_indices.emplace_back(k2);
				meshDataComponent->m_indices.emplace_back(k2 + 1);
			}
		}
	}

	meshDataComponent->m_indicesSize = meshDataComponent->m_indices.size();
}

void InnoAssetSystemNS::addTerrain(MeshDataComponent* meshDataComponent)
{
	auto l_gridSize = 1024;
	auto l_gridSize2 = l_gridSize * l_gridSize;
	auto l_gridSizehalf = l_gridSize / 2;
	meshDataComponent->m_vertices.reserve(l_gridSize2 * 4);
	meshDataComponent->m_indices.reserve(l_gridSize2 * 6);

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
			meshDataComponent->m_vertices.emplace_back(l_VertexData_1);

			Vertex l_VertexData_2;
			l_VertexData_2.m_pos = Vec4(l_px0, 0.0f, l_pz1, 1.0f);
			l_VertexData_2.m_texCoord = Vec2(l_tx0, l_tz1);
			meshDataComponent->m_vertices.emplace_back(l_VertexData_2);

			Vertex l_VertexData_3;
			l_VertexData_3.m_pos = Vec4(l_px1, 0.0f, l_pz1, 1.0f);
			l_VertexData_3.m_texCoord = Vec2(l_tx1, l_tz1);
			meshDataComponent->m_vertices.emplace_back(l_VertexData_3);

			Vertex l_VertexData_4;
			l_VertexData_4.m_pos = Vec4(l_px1, 0.0f, l_pz0, 1.0f);
			l_VertexData_4.m_texCoord = Vec2(l_tx1, l_tz0);
			meshDataComponent->m_vertices.emplace_back(l_VertexData_4);

			auto l_gridIndex = 4 * (i)+4 * l_gridSize * (j);
			meshDataComponent->m_indices.emplace_back(0 + l_gridIndex);
			meshDataComponent->m_indices.emplace_back(1 + l_gridIndex);
			meshDataComponent->m_indices.emplace_back(3 + l_gridIndex);
			meshDataComponent->m_indices.emplace_back(1 + l_gridIndex);
			meshDataComponent->m_indices.emplace_back(2 + l_gridIndex);
			meshDataComponent->m_indices.emplace_back(3 + l_gridIndex);
		}
	}
	meshDataComponent->m_indicesSize = meshDataComponent->m_indices.size();
}

bool InnoAssetSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->getComponentManager()->RegisterType<VisibleComponent>(32768);

	f_LoadModelTask = [=](VisibleComponent* i, bool AsyncLoad)
	{
		i->m_model = loadModel(i->m_modelFileName.c_str(), AsyncLoad);
	};

	f_AssignProceduralModelTask = [=](VisibleComponent* i, bool AsyncLoad)
	{
		i->m_model = addProceduralModel(i->m_proceduralMeshShape, ShaderModel::Opaque);
		auto l_pair = getMeshMaterialPair(i->m_model->meshMaterialPairs.m_startOffset);
		g_Engine->getRenderingFrontend()->registerMaterialDataComponent(l_pair->material, AsyncLoad);
	};

	// @TODO: Concurrency
	f_GeneratePDCTask = [=](VisibleComponent* i)
	{
		g_Engine->getPhysicsSystem()->generatePhysicsProxy(i);
		i->m_ObjectStatus = ObjectStatus::Activated;
	};

	m_meshMaterialPairPool = TObjectPool<MeshMaterialPair>::Create(65536);
	m_meshMaterialPairList.reserve(65536);
	m_modelPool = TObjectPool<Model>::Create(4096);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoAssetSystem::Initialize()
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

bool InnoAssetSystem::Update()
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

bool InnoAssetSystem::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "AssetSystem has been terminated.");
	return true;
}

ObjectStatus InnoAssetSystem::GetStatus()
{
	return m_ObjectStatus;
}

bool InnoAssetSystem::convertModel(const char* fileName, const char* exportPath)
{
	auto l_extension = IOService::getFileExtension(fileName);
	std::string l_fileName = fileName;

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".md5mesh")
	{
		auto tempTask = g_Engine->getTaskSystem()->submit("ConvertModelTask", -1, nullptr, [=]()
			{
				AssimpWrapper::convertModel(l_fileName.c_str(), exportPath);
			});
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Warning, "FileSystem: ", fileName, " is not supported!");

		return false;
	}
}

Model* InnoAssetSystem::loadModel(const char* fileName, bool AsyncUploadGPUResource)
{
	auto l_extension = IOService::getFileExtension(fileName);
	if (l_extension == ".InnoModel")
	{
		Model* l_loadedModel;

		if (findLoadedModel(fileName, l_loadedModel))
		{
			return l_loadedModel;
		}
		else
		{
			auto l_result = JSONWrapper::loadModelFromDisk(fileName, AsyncUploadGPUResource);
			recordLoadedModel(fileName, l_result);

			return l_result;
		}
	}
	else
	{
		InnoLogger::Log(LogLevel::Warning, "AssetSystem: ", fileName, " is not supported!");
		return nullptr;
	}
}

TextureDataComponent* InnoAssetSystem::loadTexture(const char* fileName)
{
	TextureDataComponent* l_TDC;

	if (findLoadedTexture(fileName, l_TDC))
	{
		return l_TDC;
	}
	else
	{
		l_TDC = STBWrapper::loadTexture(fileName);
		if (l_TDC)
		{
			recordLoadedTexture(fileName, l_TDC);
		}
		return l_TDC;
	}
}

bool InnoAssetSystem::saveTexture(const char* fileName, TextureDataComponent* TDC)
{
	return STBWrapper::saveTexture(fileName, TDC);
}

bool InnoAssetSystem::loadAssetsForComponents(bool AsyncLoad)
{
	auto l_visibleComponents = g_Engine->getComponentManager()->GetAll<VisibleComponent>();

	// @TODO: Load unit model first
	for (auto i : l_visibleComponents)
	{
		if (i->m_meshSource != MeshSource::Customized)
		{
			if (AsyncLoad)
			{
				auto l_loadModelTask = g_Engine->getTaskSystem()->submit("AssignProceduralModelTask", 4, nullptr, f_AssignProceduralModelTask, i, true);
				g_Engine->getTaskSystem()->submit("PDCTask", 4, l_loadModelTask, f_GeneratePDCTask, i);
			}
			else
			{
				f_AssignProceduralModelTask(i, false);
				f_GeneratePDCTask(i);
			}
		}
		else
		{
			if (!i->m_modelFileName.empty())
			{
				if (AsyncLoad)
				{
					auto l_loadModelTask = g_Engine->getTaskSystem()->submit("LoadModelTask", 4, nullptr, f_LoadModelTask, i, true);
					g_Engine->getTaskSystem()->submit("PDCTask", 4, l_loadModelTask, f_GeneratePDCTask, i);
				}
				else
				{
					f_LoadModelTask(i, false);
					f_GeneratePDCTask(i);
				}
			}
			else
			{
				InnoLogger::Log(LogLevel::Warning, "VisibleComponentManager: Custom shape mesh specified without a model preset file.");
			}
		}
	}

	return true;
}

recordLoaded(MeshMaterialPair, MeshMaterialPair*, pair)
findLoaded(MeshMaterialPair, MeshMaterialPair*&, pair)

recordLoaded(Model, Model*, model)
findLoaded(Model, Model*&, model)

recordLoaded(Texture, TextureDataComponent*, texture)
findLoaded(Texture, TextureDataComponent*&, texture)

recordLoaded(Skeleton, SkeletonDataComponent*, skeleton)
findLoaded(Skeleton, SkeletonDataComponent*&, skeleton)

recordLoaded(Animation, AnimationDataComponent*, animation)
findLoaded(Animation, AnimationDataComponent*&, animation)

ArrayRangeInfo InnoAssetSystem::addMeshMaterialPairs(uint64_t count)
{
	std::unique_lock<std::shared_mutex> lock{ m_mutexMeshMaterialPair };

	ArrayRangeInfo l_result;
	l_result.m_startOffset = m_currentMeshMaterialPairIndex;
	l_result.m_count = count;

	m_currentMeshMaterialPairIndex += count;

	for (size_t i = 0; i < count; i++)
	{
		m_meshMaterialPairList.emplace_back(m_meshMaterialPairPool->Spawn());
	}

	return l_result;
}

MeshMaterialPair* InnoAssetSystem::getMeshMaterialPair(uint64_t index)
{
	return m_meshMaterialPairList[index];
}

Model* InnoAssetSystem::addModel()
{
	std::unique_lock<std::shared_mutex> lock{ m_mutexModel };

	return m_modelPool->Spawn();
}

Model* InnoAssetSystem::addProceduralModel(ProceduralMeshShape shape, ShaderModel shaderModel)
{
	auto l_mesh = g_Engine->getRenderingFrontend()->getMeshDataComponent(shape);
	auto l_material = g_Engine->getRenderingFrontend()->addMaterialDataComponent();
	l_material->m_ShaderModel = shaderModel;
	l_material->m_ObjectStatus = ObjectStatus::Created;

	auto l_result = addModel();
	l_result->meshMaterialPairs = addMeshMaterialPairs(1);

	auto l_pair = getMeshMaterialPair(l_result->meshMaterialPairs.m_startOffset);
	l_pair->mesh = l_mesh;
	l_pair->material = l_material;

	return l_result;
}

bool InnoAssetSystem::generateProceduralMesh(ProceduralMeshShape shape, MeshDataComponent* meshDataComponent)
{
	switch (shape)
	{
	case InnoType::ProceduralMeshShape::Triangle:
		addTriangle(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Square:
		addSquare(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Pentagon:
		addPentagon(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Hexagon:
		addHexagon(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Tetrahedron:
		addTetrahedron(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Cube:
		addCube(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Octahedron:
		addOctahedron(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Dodecahedron:
		addDodecahedron(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Icosahedron:
		addIcosahedron(meshDataComponent);
		break;
	case InnoType::ProceduralMeshShape::Sphere:
		addSphere(meshDataComponent);
		break;
	default:
		break;
	}
	return true;
}