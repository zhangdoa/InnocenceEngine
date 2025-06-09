#include "TemplateAssetService.h"

#include "../Common/TaskScheduler.h"
#include "AssetService.h"
#include "../Services/ComponentManager.h"
#include "../Services/EntityManager.h"
#include "../Common/IOService.h"
#include "../ThirdParty/STBWrapper/STBWrapper.h"
#include "../Engine.h"
using namespace Inno;

namespace Inno
{
    struct TemplateAssetServiceImpl
    {
        bool LoadTemplateAssets();
        bool UnloadTemplateAssets();

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

        std::unordered_map<MeshComponent*, std::vector<Vertex>> m_meshVertices;
        std::unordered_map<MeshComponent*, std::vector<Index>> m_meshIndices;
        std::unordered_map<TextureComponent*, void*> m_textureData;

        TextureComponent* m_basicNormalTexture;
        TextureComponent* m_basicAlbedoTexture;
        TextureComponent* m_basicMetallicTexture;
        TextureComponent* m_basicRoughnessTexture;
        TextureComponent* m_basicAOTexture;

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
    auto entityManager = g_Engine->Get<EntityManager>();
    auto componentManager = g_Engine->Get<ComponentManager>();
    auto ioService = g_Engine->Get<IOService>();

    // Create template entity for engine assets
    auto templateEntity = entityManager->Spawn(false, ObjectLifespan::Frame, "TemplateAssets");

    // Load basic textures, notice we use STBWrapper to load textures directly
    m_basicNormalTexture = componentManager->Spawn<TextureComponent>(templateEntity, true, ObjectLifespan::Scene);
    std::string normalTexturePath = "Res/Textures/basic_normal.png";
    m_textureData[m_basicNormalTexture] = STBWrapper::Load(normalTexturePath.c_str(), *m_basicNormalTexture);
    if (!m_textureData[m_basicNormalTexture])
        return false;
    m_basicNormalTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
    m_basicNormalTexture->m_TextureDesc.Usage = TextureUsage::Sample;

    m_basicAlbedoTexture = componentManager->Spawn<TextureComponent>(templateEntity, true, ObjectLifespan::Scene);
    std::string albedoTexturePath = "Res/Textures/basic_albedo.png";
    m_textureData[m_basicAlbedoTexture] = STBWrapper::Load(albedoTexturePath.c_str(), *m_basicAlbedoTexture);
    if (!m_textureData[m_basicAlbedoTexture])
        return false;
    m_basicAlbedoTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
    m_basicAlbedoTexture->m_TextureDesc.Usage = TextureUsage::Sample;

    m_basicMetallicTexture = componentManager->Spawn<TextureComponent>(templateEntity, true, ObjectLifespan::Scene);
    std::string metallicTexturePath = "Res/Textures/basic_metallic.png";
    m_textureData[m_basicMetallicTexture] = STBWrapper::Load(metallicTexturePath.c_str(), *m_basicMetallicTexture);
    if (!m_textureData[m_basicMetallicTexture])
        return false;
    m_basicMetallicTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
    m_basicMetallicTexture->m_TextureDesc.Usage = TextureUsage::Sample;

    m_basicRoughnessTexture = componentManager->Spawn<TextureComponent>(templateEntity, true, ObjectLifespan::Scene);
    std::string roughnessTexturePath = "Res/Textures/basic_roughness.png";
    m_textureData[m_basicRoughnessTexture] = STBWrapper::Load(roughnessTexturePath.c_str(), *m_basicRoughnessTexture);
    if (!m_textureData[m_basicRoughnessTexture])
        return false;
    m_basicRoughnessTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
    m_basicRoughnessTexture->m_TextureDesc.Usage = TextureUsage::Sample;

    m_basicAOTexture = componentManager->Spawn<TextureComponent>(templateEntity, true, ObjectLifespan::Scene);
    std::string aoTexturePath = "Res/Textures/basic_ao.png";
    m_textureData[m_basicAOTexture] = STBWrapper::Load(aoTexturePath.c_str(), *m_basicAOTexture);
    if (!m_textureData[m_basicAOTexture])
        return false;
    m_basicAOTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
    m_basicAOTexture->m_TextureDesc.Usage = TextureUsage::Sample;

    // Create default material
    m_defaultMaterial = componentManager->Spawn<MaterialComponent>(templateEntity, true, ObjectLifespan::Scene);
    m_defaultMaterial->m_TextureComponents.reserve(5);
    m_defaultMaterial->m_TextureComponents.fulfill();
    m_defaultMaterial->m_TextureComponents[0] = m_basicNormalTexture->m_UUID;
    m_defaultMaterial->m_TextureComponents[1] = m_basicAlbedoTexture->m_UUID;
    m_defaultMaterial->m_TextureComponents[2] = m_basicMetallicTexture->m_UUID;
    m_defaultMaterial->m_TextureComponents[3] = m_basicRoughnessTexture->m_UUID;
    m_defaultMaterial->m_TextureComponents[4] = m_basicAOTexture->m_UUID;

    m_defaultMaterial->m_ShaderModel = ShaderModel::Opaque;
    AssetService::Save("default_material.MaterialComponent.inno", *m_defaultMaterial);

    // Load icon textures for lights
    m_iconTemplate_DirectionalLight = nullptr; // TODO: Implement icon loading
    m_iconTemplate_PointLight = nullptr;
    m_iconTemplate_SphereLight = nullptr;

    // Create primitive meshes
    m_unitTriangleMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Triangle, m_unitTriangleMesh);
    m_unitTriangleMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_triangle.MeshComponent.inno",
        *m_unitTriangleMesh, m_meshVertices[m_unitTriangleMesh], m_meshIndices[m_unitTriangleMesh]);

    m_unitSquareMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Square, m_unitSquareMesh);
    m_unitSquareMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_square.MeshComponent.inno",
        *m_unitSquareMesh, m_meshVertices[m_unitSquareMesh], m_meshIndices[m_unitSquareMesh]);

    m_unitPentagonMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Pentagon, m_unitPentagonMesh);
    m_unitPentagonMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_pentagon.MeshComponent.inno",
        *m_unitPentagonMesh, m_meshVertices[m_unitPentagonMesh], m_meshIndices[m_unitPentagonMesh]);

    m_unitHexagonMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Hexagon, m_unitHexagonMesh);
    m_unitHexagonMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_hexagon.MeshComponent.inno",
        *m_unitHexagonMesh, m_meshVertices[m_unitHexagonMesh], m_meshIndices[m_unitHexagonMesh]);

    m_unitTetrahedronMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Tetrahedron, m_unitTetrahedronMesh);
    m_unitTetrahedronMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_tetrahedron.MeshComponent.inno",
        *m_unitTetrahedronMesh, m_meshVertices[m_unitTetrahedronMesh], m_meshIndices[m_unitTetrahedronMesh]);

    m_unitCubeMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Cube, m_unitCubeMesh);
    m_unitCubeMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_cube.MeshComponent.inno",
        *m_unitCubeMesh, m_meshVertices[m_unitCubeMesh], m_meshIndices[m_unitCubeMesh]);


    m_unitOctahedronMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Octahedron, m_unitOctahedronMesh);
    m_unitOctahedronMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_octahedron.MeshComponent.inno",
        *m_unitOctahedronMesh, m_meshVertices[m_unitOctahedronMesh], m_meshIndices[m_unitOctahedronMesh]);

    m_unitDodecahedronMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Dodecahedron, m_unitDodecahedronMesh);
    m_unitDodecahedronMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_dodecahedron.MeshComponent.inno",
        *m_unitDodecahedronMesh, m_meshVertices[m_unitDodecahedronMesh], m_meshIndices[m_unitDodecahedronMesh]);

    m_unitIcosahedronMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Icosahedron, m_unitIcosahedronMesh);
    m_unitIcosahedronMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_icosahedron.MeshComponent.inno",
        *m_unitIcosahedronMesh, m_meshVertices[m_unitIcosahedronMesh], m_meshIndices[m_unitIcosahedronMesh]);

    m_unitSphereMesh = componentManager->Spawn<MeshComponent>(templateEntity, true, ObjectLifespan::Scene);
    GenerateMesh(MeshShape::Sphere, m_unitSphereMesh);
    m_unitSphereMesh->m_ObjectStatus = ObjectStatus::Created;
    AssetService::Save("Data/Engine/unit_sphere.MeshComponent.inno",
        *m_unitSphereMesh, m_meshVertices[m_unitSphereMesh], m_meshIndices[m_unitSphereMesh]);

    // Skip terrain for initial implementation
    m_terrainMesh = nullptr; // TODO: Implement terrain generation in the future

    // Initialize components with rendering server
    ITask::Desc taskDesc("Template Assets Initialization Task", ITask::Type::Once, 2);
    auto l_DefaultAssetInitializationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc,
        [&]() {
            auto l_renderingServer = g_Engine->getRenderingServer();

            l_renderingServer->Initialize(m_unitTriangleMesh, m_meshVertices[m_unitTriangleMesh], m_meshIndices[m_unitTriangleMesh]);
            l_renderingServer->Initialize(m_unitSquareMesh, m_meshVertices[m_unitSquareMesh], m_meshIndices[m_unitSquareMesh]);
            l_renderingServer->Initialize(m_unitPentagonMesh, m_meshVertices[m_unitPentagonMesh], m_meshIndices[m_unitPentagonMesh]);
            l_renderingServer->Initialize(m_unitHexagonMesh, m_meshVertices[m_unitHexagonMesh], m_meshIndices[m_unitHexagonMesh]);
            l_renderingServer->Initialize(m_unitTetrahedronMesh, m_meshVertices[m_unitTetrahedronMesh], m_meshIndices[m_unitTetrahedronMesh]);
            l_renderingServer->Initialize(m_unitCubeMesh, m_meshVertices[m_unitCubeMesh], m_meshIndices[m_unitCubeMesh]);
            l_renderingServer->Initialize(m_unitOctahedronMesh, m_meshVertices[m_unitOctahedronMesh], m_meshIndices[m_unitOctahedronMesh]);
            l_renderingServer->Initialize(m_unitDodecahedronMesh, m_meshVertices[m_unitDodecahedronMesh], m_meshIndices[m_unitDodecahedronMesh]);
            l_renderingServer->Initialize(m_unitIcosahedronMesh, m_meshVertices[m_unitIcosahedronMesh], m_meshIndices[m_unitIcosahedronMesh]);
            l_renderingServer->Initialize(m_unitSphereMesh, m_meshVertices[m_unitSphereMesh], m_meshIndices[m_unitSphereMesh]);
            // l_renderingServer->Initialize(m_terrainMesh, m_meshVertices[m_terrainMesh], m_meshIndices[m_terrainMesh]);

            l_renderingServer->Initialize(m_basicNormalTexture);
            l_renderingServer->Initialize(m_basicAlbedoTexture);
            l_renderingServer->Initialize(m_basicMetallicTexture);
            l_renderingServer->Initialize(m_basicRoughnessTexture);
            l_renderingServer->Initialize(m_basicAOTexture);

            // Skip icon initialization for now
            l_renderingServer->Initialize(m_iconTemplate_DirectionalLight);
            l_renderingServer->Initialize(m_iconTemplate_PointLight);
            l_renderingServer->Initialize(m_iconTemplate_SphereLight);

            l_renderingServer->Initialize(m_defaultMaterial);
        });


    l_DefaultAssetInitializationTask->Activate();
    l_DefaultAssetInitializationTask->Wait();

    return true;
}

bool TemplateAssetServiceImpl::UnloadTemplateAssets()
{
    ITask::Desc taskDesc("Template Assets Termination Task", ITask::Type::Once, 2);
    auto l_DefaultAssetTerminationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc,
        [&]() {
            auto l_renderingServer = g_Engine->getRenderingServer();

            l_renderingServer->Delete(m_basicNormalTexture);
            l_renderingServer->Delete(m_basicAlbedoTexture);
            l_renderingServer->Delete(m_basicMetallicTexture);
            l_renderingServer->Delete(m_basicRoughnessTexture);
            l_renderingServer->Delete(m_basicAOTexture);

            l_renderingServer->Delete(m_defaultMaterial);

            // Skip icon deletion for now
            l_renderingServer->Delete(m_iconTemplate_DirectionalLight);
            l_renderingServer->Delete(m_iconTemplate_PointLight);
            l_renderingServer->Delete(m_iconTemplate_SphereLight);

            l_renderingServer->Delete(m_unitTriangleMesh);
            l_renderingServer->Delete(m_unitSquareMesh);
            l_renderingServer->Delete(m_unitPentagonMesh);
            l_renderingServer->Delete(m_unitHexagonMesh);
            l_renderingServer->Delete(m_unitTetrahedronMesh);
            l_renderingServer->Delete(m_unitCubeMesh);
            l_renderingServer->Delete(m_unitOctahedronMesh);
            l_renderingServer->Delete(m_unitDodecahedronMesh);
            l_renderingServer->Delete(m_unitIcosahedronMesh);
            l_renderingServer->Delete(m_unitSphereMesh);
        });

    l_DefaultAssetTerminationTask->Activate();
    l_DefaultAssetTerminationTask->Wait();

    return true;
}

void TemplateAssetServiceImpl::generateVerticesForPolygon(MeshComponent* meshComponent, uint32_t sectorCount)
{
    auto& vertices = m_meshVertices[meshComponent];
    vertices.resize(sectorCount);

    auto l_sectorCount = (float)sectorCount;
    auto l_sectorStep = 2.0f * PI<float> / l_sectorCount;

    for (size_t i = 0; i < sectorCount; i++)
    {
        auto l_pos = Vec3(-sinf(l_sectorStep * (float)i), cosf(l_sectorStep * (float)i), 0.0f);
        vertices[i].m_pos = l_pos;
        vertices[i].m_texCoord = Vec2(l_pos.x, l_pos.y) * 0.5f + 0.5f;
    }
}

void TemplateAssetServiceImpl::generateIndicesForPolygon(MeshComponent* meshComponent, uint32_t sectorCount)
{
    auto& indices = m_meshIndices[meshComponent];
    indices.resize((sectorCount - 2) * 3);

    uint32_t l_currentIndex = 0;
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        indices[i] = l_currentIndex;
        indices[i + 1] = l_currentIndex + 1;
        indices[i + 2] = sectorCount - 1;
        l_currentIndex++;
    }
}

void TemplateAssetServiceImpl::addTriangle(MeshComponent* meshComponent)
{
    auto& vertices = m_meshVertices[meshComponent];
    auto& indices = m_meshIndices[meshComponent];

    vertices.reserve(3);
    vertices.resize(3);

    vertices[0].m_pos = Vec3(0.0f, 1.0f, 0.0f);
    vertices[0].m_texCoord = Vec2(0.5f, 1.0f);
    vertices[0].m_normal = Vec3(0.0f, 0.0f, 1.0f);

    vertices[1].m_pos = Vec3(-1.0f, -1.0f, 0.0f);
    vertices[1].m_texCoord = Vec2(0.0f, 0.0f);
    vertices[1].m_normal = Vec3(0.0f, 0.0f, 1.0f);

    vertices[2].m_pos = Vec3(1.0f, -1.0f, 0.0f);
    vertices[2].m_texCoord = Vec2(1.0f, 0.0f);
    vertices[2].m_normal = Vec3(0.0f, 0.0f, 1.0f);

    indices.reserve(3);
    indices.resize(3);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
}

void TemplateAssetServiceImpl::addSquare(MeshComponent* meshComponent)
{
    auto& vertices = m_meshVertices[meshComponent];
    auto& indices = m_meshIndices[meshComponent];

    vertices.reserve(4);
    vertices.resize(4);

    vertices[0].m_pos = Vec3(1.0f, 1.0f, 0.0f);
    vertices[0].m_texCoord = Vec2(1.0f, 1.0f);
    vertices[0].m_normal = Vec3(0.0f, 0.0f, 1.0f);

    vertices[1].m_pos = Vec3(1.0f, -1.0f, 0.0f);
    vertices[1].m_texCoord = Vec2(1.0f, 0.0f);
    vertices[1].m_normal = Vec3(0.0f, 0.0f, 1.0f);

    vertices[2].m_pos = Vec3(-1.0f, -1.0f, 0.0f);
    vertices[2].m_texCoord = Vec2(0.0f, 0.0f);
    vertices[2].m_normal = Vec3(0.0f, 0.0f, 1.0f);

    vertices[3].m_pos = Vec3(-1.0f, 1.0f, 0.0f);
    vertices[3].m_texCoord = Vec2(0.0f, 1.0f);
    vertices[3].m_normal = Vec3(0.0f, 0.0f, 1.0f);

    indices.reserve(6);
    indices.resize(6);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 3;
    indices[3] = 1;
    indices[4] = 2;
    indices[5] = 3;
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
    auto& vertices = m_meshVertices[meshComponent];
    for (size_t i = 0; i < vertices.size(); i++)
    {
        vertices[i].m_normal = vertices[i].m_pos.normalize();
    }
}

void TemplateAssetServiceImpl::generateFaceBasedNormal(MeshComponent* meshComponent, uint32_t verticesPerFace)
{
    auto& vertices = m_meshVertices[meshComponent];
    auto& indices = m_meshIndices[meshComponent];
    auto l_face = indices.size() / verticesPerFace;

    for (size_t i = 0; i < l_face; i++)
    {
        auto l_normal = Vec3(0.0f, 0.0f, 0.0f);

        for (size_t j = 0; j < verticesPerFace; j++)
        {
            l_normal = l_normal + vertices[i * verticesPerFace + j].m_pos;
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
            vertices[i * verticesPerFace + j].m_normal = l_normal;
            vertices[i * verticesPerFace + j].m_tangent = l_tangent;
        }
    }
}

void TemplateAssetServiceImpl::FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace)
{
    auto& serviceVertices = m_meshVertices[meshComponent];
    auto& serviceIndices = m_meshIndices[meshComponent];

    serviceVertices.reserve(indices.size());
    serviceVertices.resize(indices.size());

    for (uint32_t i = 0; i < indices.size(); i++)
    {
        serviceVertices[i].m_pos = vertices[indices[i]];
        // Generate UV coordinates based on position
        serviceVertices[i].m_texCoord = Vec2(
            (serviceVertices[i].m_pos.x + 1.0f) * 0.5f,
            (serviceVertices[i].m_pos.y + 1.0f) * 0.5f
        );
    }

    serviceIndices.reserve(indices.size());
    serviceIndices.resize(indices.size());
    for (uint32_t i = 0; i < indices.size(); i++)
    {
        serviceIndices[i] = i;
    }

    // Generate normals
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
    // TODO: Implement proper dodecahedron - use cube for now
    addCube(meshComponent);
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
    auto& vertices = m_meshVertices[meshComponent];
    auto& indices = m_meshIndices[meshComponent];

    auto radius = 1.0f;
    auto sectorCount = 16; // Reduced for simpler mesh
    auto stackCount = 16;

    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float s, t;

    float sectorStep = 2 * PI<float> / sectorCount;
    float stackStep = PI<float> / stackCount;
    float sectorAngle, stackAngle;

    vertices.reserve((stackCount + 1) * (sectorCount + 1));
    vertices.clear();

    for (int32_t i = 0; i <= stackCount; ++i)
    {
        stackAngle = PI<float> / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for (int32_t j = 0; j <= sectorCount; ++j)
        {
            sectorAngle = j * sectorStep;

            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);

            Vertex l_VertexData;
            l_VertexData.m_pos = Vec3(x, y, z);
            l_VertexData.m_normal = Vec3(x * lengthInv, y * lengthInv, z * lengthInv);
            l_VertexData.m_texCoord = Vec2((float)j / sectorCount, (float)i / stackCount);

            vertices.emplace_back(l_VertexData);
        }
    }

    indices.reserve(stackCount * sectorCount * 6);
    indices.clear();

    int32_t k1, k2;
    for (int32_t i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1);
        k2 = k1 + sectorCount + 1;

        for (int32_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.emplace_back(k1);
                indices.emplace_back(k2);
                indices.emplace_back(k1 + 1);
            }

            if (i != (stackCount - 1))
            {
                indices.emplace_back(k1 + 1);
                indices.emplace_back(k2);
                indices.emplace_back(k2 + 1);
            }
        }
    }
}

void TemplateAssetServiceImpl::addTerrain(MeshComponent* meshComponent)
{
	auto l_gridSize = 1024;
	auto l_gridSize2 = l_gridSize * l_gridSize;
	auto l_gridSizehalf = l_gridSize / 2;

    auto& vertices = m_meshVertices[meshComponent];
    auto& indices = m_meshIndices[meshComponent];
	vertices.reserve(l_gridSize2 * 4);
	indices.reserve(l_gridSize2 * 6);

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
			vertices.emplace_back(l_VertexData_1);

			Vertex l_VertexData_2;
			l_VertexData_2.m_pos = Vec3(l_px0, 0.0f, l_pz1);
			l_VertexData_2.m_texCoord = Vec2(l_tx0, l_tz1);
			vertices.emplace_back(l_VertexData_2);

			Vertex l_VertexData_3;
			l_VertexData_3.m_pos = Vec3(l_px1, 0.0f, l_pz1);
			l_VertexData_3.m_texCoord = Vec2(l_tx1, l_tz1);
			vertices.emplace_back(l_VertexData_3);

			Vertex l_VertexData_4;
			l_VertexData_4.m_pos = Vec3(l_px1, 0.0f, l_pz0);
			l_VertexData_4.m_texCoord = Vec2(l_tx1, l_tz0);
			vertices.emplace_back(l_VertexData_4);

			auto l_gridIndex = 4 * (i)+4 * l_gridSize * (j);
			indices.emplace_back(0 + l_gridIndex);
			indices.emplace_back(1 + l_gridIndex);
			indices.emplace_back(3 + l_gridIndex);
			indices.emplace_back(1 + l_gridIndex);
			indices.emplace_back(2 + l_gridIndex);
			indices.emplace_back(3 + l_gridIndex);
		}
	}
}

bool TemplateAssetServiceImpl::GenerateMesh(MeshShape shape, MeshComponent* meshComponent)
{
    switch (shape)
    {
    case MeshShape::Triangle:
        addTriangle(meshComponent);
        break;
    case MeshShape::Square:
        addSquare(meshComponent);
        break;
    case MeshShape::Pentagon:
        addPentagon(meshComponent);
        break;
    case MeshShape::Hexagon:
        addHexagon(meshComponent);
        break;
    case MeshShape::Tetrahedron:
        addTetrahedron(meshComponent);
        break;
    case MeshShape::Cube:
        addCube(meshComponent);
        break;
    case MeshShape::Octahedron:
        addOctahedron(meshComponent);
        break;
    case MeshShape::Dodecahedron:
        addDodecahedron(meshComponent);
        break;
    case MeshShape::Icosahedron:
        addIcosahedron(meshComponent);
        break;
    case MeshShape::Sphere:
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
    m_Impl->UnloadTemplateAssets();
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