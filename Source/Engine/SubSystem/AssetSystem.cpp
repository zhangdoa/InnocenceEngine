#include "AssetSystem.h"
#include "../Common/ComponentHeaders.h"
#include "../Common/MathHelper.h"
#include "../Core/Logger.h"
#include "../Core/IOService.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"
#include "../ThirdParty/STBWrapper/STBWrapper.h"
#include "../ThirdParty/AssimpWrapper/AssimpWrapper.h"

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

#define recordLoaded( funcName, type, value ) \
bool AssetSystem::RecordLoaded##funcName(const char * fileName, type value) \
{ \
	m_loaded##funcName.emplace(fileName, value); \
\
	return true; \
}

#define findLoaded( funcName, type, value ) \
bool AssetSystem::FindLoaded##funcName(const char * fileName, type value) \
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
	Logger::Log(LogLevel::Verbose, "AssetSystem: ", fileName, " has not been loaded."); \
	\
	return false; \
	} \
}

namespace AssetSystemNS
{
	void generateVertexBasedNormal(MeshComponent* meshComponent);
	void generateFaceBasedNormal(MeshComponent* meshComponent, uint32_t verticesPerFace);
	void FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace = 0);
	void addTriangle(MeshComponent* meshComponent);
	void addSquare(MeshComponent* meshComponent);
	void addPentagon(MeshComponent* meshComponent);
	void addHexagon(MeshComponent* meshComponent);
	void addTetrahedron(MeshComponent* meshComponent);
	void addCube(MeshComponent* meshComponent);
	void addOctahedron(MeshComponent* meshComponent);
	void addDodecahedron(MeshComponent* meshComponent);
	void addIcosahedron(MeshComponent* meshComponent);
	void addSphere(MeshComponent* meshComponent);
	void addTerrain(MeshComponent* meshComponent);

	std::function<void(VisibleComponent*, bool)> f_LoadModelTask;
	std::function<void(VisibleComponent*, bool)> f_AssignProceduralModelTask;
	std::function<void(VisibleComponent*)> f_GeneratePDCTask;

	TObjectPool<MeshMaterialPair>* m_meshMaterialPairPool;
	TObjectPool<Model>* m_modelPool;
	std::vector<MeshMaterialPair*> m_meshMaterialPairList;

	std::unordered_map<std::string, MeshMaterialPair*> m_loadedMeshMaterialPair;
	std::unordered_map<std::string, Model*> m_loadedModel;
	std::unordered_map<std::string, TextureComponent*> m_loadedTexture;
	std::unordered_map<std::string, AnimationComponent*> m_loadedAnimation;
	std::unordered_map<std::string, SkeletonComponent*> m_loadedSkeleton;

	std::atomic<uint64_t> m_currentMeshMaterialPairIndex = 0;
	std::shared_mutex m_mutexMeshMaterialPair;
	std::shared_mutex m_mutexModel;

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

using namespace AssetSystemNS;

void generateVerticesForPolygon(MeshComponent* meshComponent, uint32_t sectorCount)
{
	meshComponent->m_Vertices.reserve(sectorCount);
	meshComponent->m_Vertices.fulfill();

	auto l_sectorCount = (float)sectorCount;
	auto l_sectorStep = 2.0f * PI<float> / l_sectorCount;

	for (size_t i = 0; i < sectorCount; i++)
	{
		auto l_pos = Vec3(-sinf(l_sectorStep * (float)i), cosf(l_sectorStep * (float)i), 0.0f);
		meshComponent->m_Vertices[i].m_pos = l_pos;
		meshComponent->m_Vertices[i].m_texCoord = Vec2(l_pos.x, l_pos.y) * 0.5f + 0.5f;
	}
}

void generateIndicesForPolygon(MeshComponent* meshComponent, uint32_t sectorCount)
{
	meshComponent->m_Indices.reserve((sectorCount - 2) * 3);
	meshComponent->m_Indices.fulfill();
	meshComponent->m_IndexCount = meshComponent->m_Indices.capacity();

	uint32_t l_currentIndex = 0;

	for (size_t i = 0; i < meshComponent->m_IndexCount; i += 3)
	{
		meshComponent->m_Indices[i] = l_currentIndex;
		meshComponent->m_Indices[i + 1] = l_currentIndex + 1;
		meshComponent->m_Indices[i + 2] = sectorCount - 1;
		l_currentIndex++;
	}
}

void AssetSystemNS::addTriangle(MeshComponent* meshComponent)
{
	generateVerticesForPolygon(meshComponent, 3);
	generateIndicesForPolygon(meshComponent, 3);
}

void AssetSystemNS::addSquare(MeshComponent* meshComponent)
{
	meshComponent->m_Vertices.reserve(4);
	meshComponent->m_Vertices.fulfill();

	meshComponent->m_Vertices[0].m_pos = Vec3(1.0f, 1.0f, 0.0f);
	meshComponent->m_Vertices[0].m_texCoord = Vec2(1.0f, 1.0f);

	meshComponent->m_Vertices[1].m_pos = Vec3(1.0f, -1.0f, 0.0f);
	meshComponent->m_Vertices[1].m_texCoord = Vec2(1.0f, 0.0f);

	meshComponent->m_Vertices[2].m_pos = Vec3(-1.0f, -1.0f, 0.0f);
	meshComponent->m_Vertices[2].m_texCoord = Vec2(0.0f, 0.0f);

	meshComponent->m_Vertices[3].m_pos = Vec3(-1.0f, 1.0f, 0.0f);
	meshComponent->m_Vertices[3].m_texCoord = Vec2(0.0f, 1.0f);

	meshComponent->m_Indices.reserve(6);
	meshComponent->m_Indices.fulfill();

	meshComponent->m_Indices[0] = 0;
	meshComponent->m_Indices[1] = 1;
	meshComponent->m_Indices[2] = 3;
	meshComponent->m_Indices[3] = 1;
	meshComponent->m_Indices[4] = 2;
	meshComponent->m_Indices[5] = 3;

	meshComponent->m_IndexCount = meshComponent->m_Indices.size();
}

void AssetSystemNS::addPentagon(MeshComponent* meshComponent)
{
	generateVerticesForPolygon(meshComponent, 5);
	generateIndicesForPolygon(meshComponent, 5);
}

void AssetSystemNS::addHexagon(MeshComponent* meshComponent)
{
	generateVerticesForPolygon(meshComponent, 6);
	generateIndicesForPolygon(meshComponent, 6);
}

void AssetSystemNS::generateVertexBasedNormal(MeshComponent* meshComponent)
{
	auto l_verticesCount = meshComponent->m_Vertices.size();
	for (size_t i = 0; i < l_verticesCount; i++)
	{
		meshComponent->m_Vertices[i].m_normal = meshComponent->m_Vertices[i].m_pos.normalize();
	}
}

void AssetSystemNS::generateFaceBasedNormal(MeshComponent* meshComponent, uint32_t verticesPerFace)
{
	auto l_face = meshComponent->m_Indices.size() / verticesPerFace;

	for (size_t i = 0; i < l_face; i++)
	{
		auto l_normal = Vec3(0.0f, 0.0f, 0.0f);

		for (size_t j = 0; j < verticesPerFace; j++)
		{
			l_normal = l_normal + meshComponent->m_Vertices[i * verticesPerFace + j].m_pos;
		}
		l_normal = l_normal / (float)verticesPerFace;
		l_normal = l_normal.normalize();

		auto l_up = Vec3(0.0f, 1.0f, 0.0f);
		auto l_tangent = Vec3();
		if (l_normal != l_up)
		{
			l_tangent = l_up.cross(l_normal);
			l_tangent = l_tangent.normalize();
		}
		else
		{
			auto l_right = Vec3(1.0f, 0.0f, 0.0f);
			l_tangent = l_normal.cross(l_right);
			l_tangent = l_tangent.normalize();
		}

		for (size_t j = 0; j < verticesPerFace; j++)
		{
			meshComponent->m_Vertices[i * verticesPerFace + j].m_normal = l_normal;
			meshComponent->m_Vertices[i * verticesPerFace + j].m_tangent = l_tangent;
		}
	}
}

void AssetSystemNS::FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace)
{
	meshComponent->m_Vertices.reserve(indices.size());
	meshComponent->m_Vertices.fulfill();

	for (uint32_t i = 0; i < indices.size(); i++)
	{
		meshComponent->m_Vertices[i].m_pos = vertices[indices[i]];
	}

	meshComponent->m_Indices.reserve(indices.size());
	meshComponent->m_Indices.fulfill();

	for (uint32_t i = 0; i < indices.size(); i++)
	{
		meshComponent->m_Indices[i] = i;
	}

	meshComponent->m_IndexCount = meshComponent->m_Indices.size();

	if (verticesPerFace)
	{
		generateFaceBasedNormal(meshComponent, verticesPerFace);
	}
	else
	{
		generateVertexBasedNormal(meshComponent);
	}
}

void AssetSystemNS::addTetrahedron(MeshComponent* meshComponent)
{
	std::vector<Index> l_indices =
	{
		0, 3, 1, 0, 2, 3,
		0, 1, 2, 1, 3, 2
	};

	std::vector<Vec3> l_vertices =
	{
		Vec3(1.0f, 1.0f, 1.0f),
		Vec3(1.0f, -1.0f, -1.0f),
		Vec3(-1.0f, 1.0f, -1.0f),
		Vec3(-1.0f, -1.0f, 1.0f)
	};

	FulfillVerticesAndIndices(meshComponent, l_indices, l_vertices, 3);
}

void AssetSystemNS::addCube(MeshComponent* meshComponent)
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

	std::vector<Vec3> l_vertices =
	{
		Vec3(1.0f, 1.0f, 1.0f),
		Vec3(1.0f, -1.0f, 1.0f),
		Vec3(-1.0f, -1.0f, 1.0f),
		Vec3(-1.0f, 1.0f, 1.0f),
		Vec3(1.0f, 1.0f, -1.0f),
		Vec3(1.0f, -1.0f, -1.0f),
		Vec3(-1.0f, -1.0f, -1.0f),
		Vec3(-1.0f, 1.0f, -1.0f)
	};

	FulfillVerticesAndIndices(meshComponent, l_indices, l_vertices, 6);
}

void AssetSystemNS::addOctahedron(MeshComponent* meshComponent)
{
	std::vector<Index> l_indices =
	{
		0, 2, 4, 4, 2, 1,
		1, 2, 5, 5, 2, 0,
		0, 4, 3, 4, 1, 3,
		1, 5, 3, 5, 0, 3
	};

	std::vector<Vec3> l_vertices =
	{
		Vec3(1.0f, 0.0f, 0.0f),
		Vec3(-1.0f, 0.0f, 0.0f),
		Vec3(0.0f, 1.0f, 0.0f),
		Vec3(0.0f, -1.0f, 0.0f),
		Vec3(0.0f, 0.0f, 1.0f),
		Vec3(0.0f, 0.0f, -1.0f)
	};

	FulfillVerticesAndIndices(meshComponent, l_indices, l_vertices, 3);
}

void AssetSystemNS::addDodecahedron(MeshComponent* meshComponent)
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

	std::vector<Vec3> l_vertices =
	{
		Vec3(0.0f, 1.61803398875f, 0.61803398875f),
		Vec3(-1.0f, 1.0f, 1.0f),
		Vec3(-0.61803398875f, 0.0f, 1.61803398875f),
		Vec3(0.61803398875f, 0.0f, 1.61803398875f),
		Vec3(1.0f, 1.0f, 1.0f),
		Vec3(0.0f, 1.61803398875f, -0.61803398875f),
		Vec3(1.0f, 1.0f, -1.0f),
		Vec3(0.61803398875f, 0.0f, -1.61803398875f),
		Vec3(-0.61803398875f, 0.0f, -1.61803398875f),
		Vec3(-1.0f, 1.0f, -1.0f),
		Vec3(0.0f, -1.61803398875f, 0.61803398875f),
		Vec3(1.0f, -1.0f, 1.0f),
		Vec3(-1.0f, -1.0f, 1.0f),
		Vec3(0.0f, -1.61803398875f, -0.61803398875f),
		Vec3(-1.0f, -1.0f, -1.0f),
		Vec3(1.0f, -1.0f, -1.0f),
		Vec3(1.61803398875f, -0.61803398875f, 0.0f),
		Vec3(1.61803398875f, 0.61803398875f, 0.0f),
		Vec3(-1.61803398875f, 0.61803398875f, 0.0f),
		Vec3(-1.61803398875f, -0.61803398875f, 0.0f)
	};

	FulfillVerticesAndIndices(meshComponent, l_indices, l_vertices, 9);
}

void AssetSystemNS::addIcosahedron(MeshComponent* meshComponent)
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

	std::vector<Vec3> l_vertices =
	{
		Vec3(1.0f, 0.0f, 0.0f),
		Vec3(0.447213595500f, 0.894427191000f, 0.0f),
		Vec3(0.447213595500f, 0.276393202252f, 0.850650808354f),
		Vec3(0.447213595500f, -0.723606797748f, 0.525731112119f),
		Vec3(0.447213595500f, -0.723606797748f, -0.525731112119f),
		Vec3(0.447213595500f, 0.276393202252f, -0.850650808354f),
		Vec3(-0.447213595500f, -0.894427191000f, 0.0f),
		Vec3(-0.447213595500f, -0.276393202252f, 0.850650808354f),
		Vec3(-0.447213595500f, 0.723606797748f, 0.525731112119f),
		Vec3(-0.447213595500f, 0.723606797748f, -0.525731112119f),
		Vec3(-0.447213595500f, -0.276393202252f, -0.850650808354f),
		Vec3(-1.0f, 0.0f, 0.0f)
	};

	FulfillVerticesAndIndices(meshComponent, l_indices, l_vertices, 3);
}

void AssetSystemNS::addSphere(MeshComponent* meshComponent)
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

	meshComponent->m_Vertices.reserve((stackCount + 1) * (sectorCount + 1));

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
			l_VertexData.m_pos = Vec3(x, y, z);

			// normalized vertex normal (nx, ny, nz)
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			l_VertexData.m_normal = Vec3(nx, ny, nz);

			// vertex tex coord (s, t) range between [0, 1]
			s = (float)j / sectorCount;
			t = (float)i / stackCount;
			l_VertexData.m_texCoord = Vec2(s, t);

			meshComponent->m_Vertices.emplace_back(l_VertexData);
		}
	}

	meshComponent->m_Indices.reserve(stackCount * (sectorCount - 1) * 6);

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
				meshComponent->m_Indices.emplace_back(k1);
				meshComponent->m_Indices.emplace_back(k2);
				meshComponent->m_Indices.emplace_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				meshComponent->m_Indices.emplace_back(k1 + 1);
				meshComponent->m_Indices.emplace_back(k2);
				meshComponent->m_Indices.emplace_back(k2 + 1);
			}
		}
	}

	meshComponent->m_IndexCount = meshComponent->m_Indices.size();
}

void AssetSystemNS::addTerrain(MeshComponent* meshComponent)
{
	auto l_gridSize = 1024;
	auto l_gridSize2 = l_gridSize * l_gridSize;
	auto l_gridSizehalf = l_gridSize / 2;
	meshComponent->m_Vertices.reserve(l_gridSize2 * 4);
	meshComponent->m_Indices.reserve(l_gridSize2 * 6);

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
			l_VertexData_1.m_pos = Vec3(l_px0, 0.0f, l_pz0);
			l_VertexData_1.m_texCoord = Vec2(l_tx0, l_tz0);
			meshComponent->m_Vertices.emplace_back(l_VertexData_1);

			Vertex l_VertexData_2;
			l_VertexData_2.m_pos = Vec3(l_px0, 0.0f, l_pz1);
			l_VertexData_2.m_texCoord = Vec2(l_tx0, l_tz1);
			meshComponent->m_Vertices.emplace_back(l_VertexData_2);

			Vertex l_VertexData_3;
			l_VertexData_3.m_pos = Vec3(l_px1, 0.0f, l_pz1);
			l_VertexData_3.m_texCoord = Vec2(l_tx1, l_tz1);
			meshComponent->m_Vertices.emplace_back(l_VertexData_3);

			Vertex l_VertexData_4;
			l_VertexData_4.m_pos = Vec3(l_px1, 0.0f, l_pz0);
			l_VertexData_4.m_texCoord = Vec2(l_tx1, l_tz0);
			meshComponent->m_Vertices.emplace_back(l_VertexData_4);

			auto l_gridIndex = 4 * (i)+4 * l_gridSize * (j);
			meshComponent->m_Indices.emplace_back(0 + l_gridIndex);
			meshComponent->m_Indices.emplace_back(1 + l_gridIndex);
			meshComponent->m_Indices.emplace_back(3 + l_gridIndex);
			meshComponent->m_Indices.emplace_back(1 + l_gridIndex);
			meshComponent->m_Indices.emplace_back(2 + l_gridIndex);
			meshComponent->m_Indices.emplace_back(3 + l_gridIndex);
		}
	}
	meshComponent->m_IndexCount = meshComponent->m_Indices.size();
}

bool AssetSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->getComponentManager()->RegisterType<VisibleComponent>(32768, this);

	f_LoadModelTask = [=](VisibleComponent* i, bool AsyncLoad)
	{
		i->m_model = LoadModel(i->m_modelFileName.c_str(), AsyncLoad);
	};

	f_AssignProceduralModelTask = [=](VisibleComponent* i, bool AsyncLoad)
	{
		i->m_model = AddProceduralModel(i->m_proceduralMeshShape, ShaderModel::Opaque);
		auto l_pair = GetMeshMaterialPair(i->m_model->meshMaterialPairs.m_startOffset);
		g_Engine->getRenderingFrontend()->InitializeMaterialComponent(l_pair->material, AsyncLoad);
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

bool AssetSystem::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		Logger::Log(LogLevel::Success, "AssetSystem has been initialized.");
		return true;
	}
	else
	{
		Logger::Log(LogLevel::Error, "AssetSystem: Object is not created!");
		return false;
	}
}

bool AssetSystem::Update()
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

bool AssetSystem::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Logger::Log(LogLevel::Success, "AssetSystem has been terminated.");
	return true;
}

ObjectStatus AssetSystem::GetStatus()
{
	return m_ObjectStatus;
}

bool AssetSystem::ConvertModel(const char* fileName, const char* exportPath)
{
	auto l_extension = IOService::getFileExtension(fileName);
	std::string l_fileName = fileName;

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".md5mesh")
	{
		auto tempTask = g_Engine->getTaskSystem()->Submit("ConvertModelTask", -1, nullptr, [=]()
			{
				AssimpWrapper::ConvertModel(l_fileName.c_str(), exportPath);
			});
		return true;
	}
	else
	{
		Logger::Log(LogLevel::Warning, "FileSystem: ", fileName, " is not supported!");

		return false;
	}
}

Model* AssetSystem::LoadModel(const char* fileName, bool AsyncUploadGPUResource)
{
	auto l_extension = IOService::getFileExtension(fileName);
	if (l_extension == ".InnoModel")
	{
		Model* l_loadedModel;

		if (FindLoadedModel(fileName, l_loadedModel))
		{
			return l_loadedModel;
		}
		else
		{
			auto l_result = JSONWrapper::loadModelFromDisk(fileName, AsyncUploadGPUResource);
			RecordLoadedModel(fileName, l_result);

			return l_result;
		}
	}
	else
	{
		Logger::Log(LogLevel::Warning, "AssetSystem: ", fileName, " is not supported!");
		return nullptr;
	}
}

TextureComponent* AssetSystem::LoadTexture(const char* fileName)
{
	TextureComponent* l_TextureComp;

	if (FindLoadedTexture(fileName, l_TextureComp))
	{
		return l_TextureComp;
	}
	else
	{
		l_TextureComp = STBWrapper::LoadTexture(fileName);
		if (l_TextureComp)
		{
			RecordLoadedTexture(fileName, l_TextureComp);
		}
		return l_TextureComp;
	}
}

bool AssetSystem::SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData)
{
    return STBWrapper::SaveTexture(fileName, textureDesc, textureData);
}

bool AssetSystem::SaveTexture(const char* fileName, TextureComponent* TextureComp)
{
	return STBWrapper::SaveTexture(fileName, TextureComp->m_TextureDesc, TextureComp->m_TextureData);
}

bool AssetSystem::LoadAssetsForComponents(bool AsyncLoad)
{
	auto l_visibleComponents = g_Engine->getComponentManager()->GetAll<VisibleComponent>();

	// @TODO: Load unit model first
	for (auto i : l_visibleComponents)
	{
		if (i->m_meshSource != MeshSource::Customized)
		{
			if (AsyncLoad)
			{
				auto l_loadModelTask = g_Engine->getTaskSystem()->Submit("AssignProceduralModelTask", 4, nullptr, f_AssignProceduralModelTask, i, true);
				g_Engine->getTaskSystem()->Submit("PDCTask", 4, l_loadModelTask.m_Task, f_GeneratePDCTask, i);
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
					auto l_loadModelTask = g_Engine->getTaskSystem()->Submit("LoadModelTask", 4, nullptr, f_LoadModelTask, i, true);
					g_Engine->getTaskSystem()->Submit("PDCTask", 4, l_loadModelTask.m_Task, f_GeneratePDCTask, i);
				}
				else
				{
					f_LoadModelTask(i, false);
					f_GeneratePDCTask(i);
				}
			}
			else
			{
				Logger::Log(LogLevel::Warning, "VisibleComponentManager: Custom shape mesh specified without a model preset file.");
			}
		}
	}

	return true;
}

recordLoaded(MeshMaterialPair, MeshMaterialPair*, pair)
findLoaded(MeshMaterialPair, MeshMaterialPair*&, pair)

recordLoaded(Model, Model*, model)
findLoaded(Model, Model*&, model)

recordLoaded(Texture, TextureComponent*, texture)
findLoaded(Texture, TextureComponent*&, texture)

recordLoaded(Skeleton, SkeletonComponent*, skeleton)
findLoaded(Skeleton, SkeletonComponent*&, skeleton)

recordLoaded(Animation, AnimationComponent*, animation)
findLoaded(Animation, AnimationComponent*&, animation)

ArrayRangeInfo AssetSystem::AddMeshMaterialPairs(uint64_t count)
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

MeshMaterialPair* AssetSystem::GetMeshMaterialPair(uint64_t index)
{
	return m_meshMaterialPairList[index];
}

Model* AssetSystem::AddModel()
{
	std::unique_lock<std::shared_mutex> lock{ m_mutexModel };

	return m_modelPool->Spawn();
}

Model* AssetSystem::AddProceduralModel(ProceduralMeshShape shape, ShaderModel shaderModel)
{
	auto l_mesh = g_Engine->getRenderingFrontend()->GetMeshComponent(shape);
	auto l_material = g_Engine->getRenderingFrontend()->AddMaterialComponent();
	l_material->m_ShaderModel = shaderModel;
	l_material->m_ObjectStatus = ObjectStatus::Created;

	auto l_result = AddModel();
	l_result->meshMaterialPairs = AddMeshMaterialPairs(1);

	auto l_pair = GetMeshMaterialPair(l_result->meshMaterialPairs.m_startOffset);
	l_pair->mesh = l_mesh;
	l_pair->material = l_material;

	return l_result;
}

bool AssetSystem::GenerateProceduralMesh(ProceduralMeshShape shape, MeshComponent* meshComponent)
{
	switch (shape)
	{
	case Type::ProceduralMeshShape::Triangle:
		addTriangle(meshComponent);
		break;
	case Type::ProceduralMeshShape::Square:
		addSquare(meshComponent);
		break;
	case Type::ProceduralMeshShape::Pentagon:
		addPentagon(meshComponent);
		break;
	case Type::ProceduralMeshShape::Hexagon:
		addHexagon(meshComponent);
		break;
	case Type::ProceduralMeshShape::Tetrahedron:
		addTetrahedron(meshComponent);
		break;
	case Type::ProceduralMeshShape::Cube:
		addCube(meshComponent);
		break;
	case Type::ProceduralMeshShape::Octahedron:
		addOctahedron(meshComponent);
		break;
	case Type::ProceduralMeshShape::Dodecahedron:
		addDodecahedron(meshComponent);
		break;
	case Type::ProceduralMeshShape::Icosahedron:
		addIcosahedron(meshComponent);
		break;
	case Type::ProceduralMeshShape::Sphere:
		addSphere(meshComponent);
		break;
	default:
		break;
	}
	return true;
}

void AssetSystem::FulfillVerticesAndIndices(MeshComponent *meshComponent, const std::vector<Index> &indices, const std::vector<Vec3> &vertices, uint32_t verticesPerFace)
{
	 AssetSystemNS::FulfillVerticesAndIndices(meshComponent, indices, vertices, verticesPerFace);
}