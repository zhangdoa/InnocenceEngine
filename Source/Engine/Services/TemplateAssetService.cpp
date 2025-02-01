#include "TemplateAssetService.h"

#include "../Common/TaskScheduler.h"
#include "AssetSystem.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
    struct TemplateAssetServiceImpl
    {
        bool LoadTemplateAssets();

        void generateVerticesForPolygon(MeshComponent* meshComponent, uint32_t sectorCount);
        void generateIndicesForPolygon(MeshComponent* meshComponent, uint32_t sectorCount);
        void generateVertexBasedNormal(MeshComponent* meshComponent);
        void generateFaceBasedNormal(MeshComponent* meshComponent, uint32_t verticesPerFace);

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

        void FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace = 0);
		bool GenerateMesh(MeshShape shape, MeshComponent* meshComponent);

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

        TextureComponent* m_iconTemplate_DirectionalLight;
        TextureComponent* m_iconTemplate_PointLight;
        TextureComponent* m_iconTemplate_SphereLight;

        MeshComponent* m_unitTriangleMesh;
        MeshComponent* m_unitSquareMesh;
        MeshComponent* m_unitPentagonMesh;
        MeshComponent* m_unitHexagonMesh;

        MeshComponent* m_unitTetrahedronMesh;
        MeshComponent* m_unitCubeMesh;
        MeshComponent* m_unitOctahedronMesh;
        MeshComponent* m_unitDodecahedronMesh;
        MeshComponent* m_unitIcosahedronMesh;
        MeshComponent* m_unitSphereMesh;
        MeshComponent* m_terrainMesh;

        MaterialComponent* m_defaultMaterial;
    };
}

bool TemplateAssetServiceImpl::LoadTemplateAssets()
{
	auto m_basicNormalTexture = g_Engine->Get<AssetSystem>()->LoadTexture("..//Res//Textures//basic_normal.png");
	m_basicNormalTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicNormalTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	auto m_basicAlbedoTexture = g_Engine->Get<AssetSystem>()->LoadTexture("..//Res//Textures//basic_albedo.png");
	m_basicAlbedoTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicAlbedoTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	auto m_basicMetallicTexture = g_Engine->Get<AssetSystem>()->LoadTexture("..//Res//Textures//basic_metallic.png");
	m_basicMetallicTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicMetallicTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	auto m_basicRoughnessTexture = g_Engine->Get<AssetSystem>()->LoadTexture("..//Res//Textures//basic_roughness.png");
	m_basicRoughnessTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicRoughnessTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	auto m_basicAOTexture = g_Engine->Get<AssetSystem>()->LoadTexture("..//Res//Textures//basic_ao.png");
	m_basicAOTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicAOTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	m_defaultMaterial = g_Engine->getRenderingServer()->AddMaterialComponent("BasicMaterial/");
	m_defaultMaterial->m_TextureSlots[0].m_Texture = m_basicNormalTexture;
	m_defaultMaterial->m_TextureSlots[1].m_Texture = m_basicAlbedoTexture;
	m_defaultMaterial->m_TextureSlots[2].m_Texture = m_basicMetallicTexture;
	m_defaultMaterial->m_TextureSlots[3].m_Texture = m_basicRoughnessTexture;
	m_defaultMaterial->m_TextureSlots[4].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[5].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[6].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[7].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_ShaderModel = ShaderModel::Opaque;

	m_iconTemplate_DirectionalLight = g_Engine->Get<AssetSystem>()->LoadTexture("..//Res//Textures//WorldEditorIcons_DirectionalLight.png");
	m_iconTemplate_DirectionalLight->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_iconTemplate_DirectionalLight->m_TextureDesc.Usage = TextureUsage::Sample;

	m_iconTemplate_PointLight = g_Engine->Get<AssetSystem>()->LoadTexture("..//Res//Textures//WorldEditorIcons_PointLight.png");
	m_iconTemplate_PointLight->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_iconTemplate_PointLight->m_TextureDesc.Usage = TextureUsage::Sample;

	m_iconTemplate_SphereLight = g_Engine->Get<AssetSystem>()->LoadTexture("..//Res//Textures//WorldEditorIcons_SphereLight.png");
	m_iconTemplate_SphereLight->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_iconTemplate_SphereLight->m_TextureDesc.Usage = TextureUsage::Sample;

	m_unitTriangleMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitTriangleMesh/");
	GenerateMesh(MeshShape::Triangle, m_unitTriangleMesh);
	m_unitTriangleMesh->m_MeshShape = MeshShape::Triangle;
	m_unitTriangleMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitSquareMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitSquareMesh/");
	GenerateMesh(MeshShape::Square, m_unitSquareMesh);
	m_unitSquareMesh->m_MeshShape = MeshShape::Square;
	m_unitSquareMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitPentagonMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitPentagonMesh/");
	GenerateMesh(MeshShape::Pentagon, m_unitPentagonMesh);
	m_unitPentagonMesh->m_MeshShape = MeshShape::Pentagon;
	m_unitPentagonMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitHexagonMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitHexagonMesh/");
	GenerateMesh(MeshShape::Hexagon, m_unitHexagonMesh);
	m_unitHexagonMesh->m_MeshShape = MeshShape::Hexagon;
	m_unitHexagonMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitTetrahedronMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitTetrahedronMesh/");
	GenerateMesh(MeshShape::Tetrahedron, m_unitTetrahedronMesh);
	m_unitTetrahedronMesh->m_MeshShape = MeshShape::Tetrahedron;
	m_unitTetrahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitCubeMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitCubeMesh/");
	GenerateMesh(MeshShape::Cube, m_unitCubeMesh);
	m_unitCubeMesh->m_MeshShape = MeshShape::Cube;
	m_unitCubeMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitOctahedronMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitOctahedronMesh/");
	GenerateMesh(MeshShape::Octahedron, m_unitOctahedronMesh);
	m_unitOctahedronMesh->m_MeshShape = MeshShape::Octahedron;
	m_unitOctahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitDodecahedronMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitDodecahedronMesh/");
	GenerateMesh(MeshShape::Dodecahedron, m_unitDodecahedronMesh);
	m_unitDodecahedronMesh->m_MeshShape = MeshShape::Dodecahedron;
	m_unitDodecahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitIcosahedronMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitIcosahedronMesh/");
	GenerateMesh(MeshShape::Icosahedron, m_unitIcosahedronMesh);
	m_unitIcosahedronMesh->m_MeshShape = MeshShape::Icosahedron;
	m_unitIcosahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitSphereMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitSphereMesh/");
	GenerateMesh(MeshShape::Sphere, m_unitSphereMesh);
	m_unitSphereMesh->m_MeshShape = MeshShape::Sphere;
	m_unitSphereMesh->m_ObjectStatus = ObjectStatus::Created;

	ITask::Desc taskDesc("Default Assets Initialization Task", ITask::Type::Once, 2);
	auto l_DefaultAssetInitializationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc,
		[&]() {
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitTriangleMesh);
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitSquareMesh);
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitPentagonMesh);
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitHexagonMesh);

			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitTetrahedronMesh);
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitCubeMesh);
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitOctahedronMesh);
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitDodecahedronMesh);
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitIcosahedronMesh);
			g_Engine->getRenderingServer()->InitializeMeshComponent(m_unitSphereMesh);

			g_Engine->getRenderingServer()->InitializeTextureComponent(m_basicNormalTexture);
			g_Engine->getRenderingServer()->InitializeTextureComponent(m_basicAlbedoTexture);
			g_Engine->getRenderingServer()->InitializeTextureComponent(m_basicMetallicTexture);
			g_Engine->getRenderingServer()->InitializeTextureComponent(m_basicRoughnessTexture);
			g_Engine->getRenderingServer()->InitializeTextureComponent(m_basicAOTexture);

			g_Engine->getRenderingServer()->InitializeTextureComponent(m_iconTemplate_DirectionalLight);
			g_Engine->getRenderingServer()->InitializeTextureComponent(m_iconTemplate_PointLight);
			g_Engine->getRenderingServer()->InitializeTextureComponent(m_iconTemplate_SphereLight);

			g_Engine->getRenderingServer()->InitializeMaterialComponent(m_defaultMaterial);
		});

	l_DefaultAssetInitializationTask->Activate();
	l_DefaultAssetInitializationTask->Wait();

	return true;
}

void TemplateAssetServiceImpl::generateVerticesForPolygon(MeshComponent* meshComponent, uint32_t sectorCount)
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

void TemplateAssetServiceImpl::generateIndicesForPolygon(MeshComponent* meshComponent, uint32_t sectorCount)
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

void TemplateAssetServiceImpl::addTriangle(MeshComponent* meshComponent)
{
	generateVerticesForPolygon(meshComponent, 3);
	generateIndicesForPolygon(meshComponent, 3);
}

void TemplateAssetServiceImpl::addSquare(MeshComponent* meshComponent)
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

void TemplateAssetServiceImpl::addPentagon(MeshComponent* meshComponent)
{
	generateVerticesForPolygon(meshComponent, 5);
	generateIndicesForPolygon(meshComponent, 5);
}

void TemplateAssetServiceImpl::addHexagon(MeshComponent* meshComponent)
{
	generateVerticesForPolygon(meshComponent, 6);
	generateIndicesForPolygon(meshComponent, 6);
}

void TemplateAssetServiceImpl::generateVertexBasedNormal(MeshComponent* meshComponent)
{
	auto l_verticesCount = meshComponent->m_Vertices.size();
	for (size_t i = 0; i < l_verticesCount; i++)
	{
		meshComponent->m_Vertices[i].m_normal = meshComponent->m_Vertices[i].m_pos.normalize();
	}
}

void TemplateAssetServiceImpl::generateFaceBasedNormal(MeshComponent* meshComponent, uint32_t verticesPerFace)
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

void TemplateAssetServiceImpl::FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace)
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

void TemplateAssetServiceImpl::addTetrahedron(MeshComponent* meshComponent)
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

void TemplateAssetServiceImpl::addCube(MeshComponent* meshComponent)
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

void TemplateAssetServiceImpl::addOctahedron(MeshComponent* meshComponent)
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

void TemplateAssetServiceImpl::addDodecahedron(MeshComponent* meshComponent)
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

void TemplateAssetServiceImpl::addIcosahedron(MeshComponent* meshComponent)
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

void TemplateAssetServiceImpl::addSphere(MeshComponent* meshComponent)
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

void TemplateAssetServiceImpl::addTerrain(MeshComponent* meshComponent)
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

bool TemplateAssetServiceImpl::GenerateMesh(MeshShape shape, MeshComponent* meshComponent)
{
	switch (shape)
	{
	case Type::MeshShape::Triangle:
		addTriangle(meshComponent);
		break;
	case Type::MeshShape::Square:
		addSquare(meshComponent);
		break;
	case Type::MeshShape::Pentagon:
		addPentagon(meshComponent);
		break;
	case Type::MeshShape::Hexagon:
		addHexagon(meshComponent);
		break;
	case Type::MeshShape::Tetrahedron:
		addTetrahedron(meshComponent);
		break;
	case Type::MeshShape::Cube:
		addCube(meshComponent);
		break;
	case Type::MeshShape::Octahedron:
		addOctahedron(meshComponent);
		break;
	case Type::MeshShape::Dodecahedron:
		addDodecahedron(meshComponent);
		break;
	case Type::MeshShape::Icosahedron:
		addIcosahedron(meshComponent);
		break;
	case Type::MeshShape::Sphere:
		addSphere(meshComponent);
		break;
	default:
		break;
	}
	return true;
}

bool TemplateAssetService::Setup(ISystemConfig* systemConfig)
{	
	m_Impl = new TemplateAssetServiceImpl();

	m_Impl->m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool TemplateAssetService::Initialize()
{
	m_Impl->LoadTemplateAssets();
	m_Impl->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool TemplateAssetService::Update()
{
	return true;
}

bool TemplateAssetService::Terminate()
{
	delete m_Impl;
	return true;
}

ObjectStatus TemplateAssetService::GetStatus()
{
	return 	m_Impl->m_ObjectStatus;
}

MeshComponent* TemplateAssetService::GetMeshComponent(MeshShape shape)
{
	switch (shape)
	{
	case MeshShape::Triangle:
		return m_Impl->m_unitTriangleMesh;
		break;
	case MeshShape::Square:
		return m_Impl->m_unitSquareMesh;
		break;
	case MeshShape::Pentagon:
		return m_Impl->m_unitPentagonMesh;
		break;
	case MeshShape::Hexagon:
		return m_Impl->m_unitHexagonMesh;
		break;
	case MeshShape::Tetrahedron:
		return m_Impl->m_unitTetrahedronMesh;
		break;
	case MeshShape::Cube:
		return m_Impl->m_unitCubeMesh;
		break;
	case MeshShape::Octahedron:
		return m_Impl->m_unitOctahedronMesh;
		break;
	case MeshShape::Dodecahedron:
		return m_Impl->m_unitDodecahedronMesh;
		break;
	case MeshShape::Icosahedron:
		return m_Impl->m_unitIcosahedronMesh;
		break;
	case MeshShape::Sphere:
		return m_Impl->m_unitSphereMesh;
		break;
	default:
		Log(Error, "Invalid MeshShape!");
		return nullptr;
		break;
	}
}

TextureComponent* TemplateAssetService::GetTextureComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return m_Impl->m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return m_Impl->m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return m_Impl->m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

MaterialComponent* TemplateAssetService::GetDefaultMaterialComponent()
{
	return m_Impl->m_defaultMaterial;
}

bool TemplateAssetService::GenerateMesh(MeshShape shape, MeshComponent* meshComponent)
{
	return m_Impl->GenerateMesh(shape, meshComponent);
}

void TemplateAssetService::FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace)
{
	return m_Impl->FulfillVerticesAndIndices(meshComponent, indices, vertices, verticesPerFace);
}