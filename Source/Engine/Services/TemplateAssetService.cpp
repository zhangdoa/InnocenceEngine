#include "TemplateAssetService.h"

#include "../Common/TaskScheduler.h"
#include "AssetService.h"
#include "EntityRegistry.h"
#include "../Common/IOService.h"
#include "../ThirdParty/STBWrapper/STBWrapper.h"
#include "../Engine.h"
#include "TextureResourceService.h"
#include "MeshResourceService.h"
#include "MaterialResourceService.h"
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

        EntityID m_basicNormalTextureEntity      = INVALID_ENTITY;
        EntityID m_basicAlbedoTextureEntity      = INVALID_ENTITY;
        EntityID m_basicMetallicTextureEntity    = INVALID_ENTITY;
        EntityID m_basicRoughnessTextureEntity   = INVALID_ENTITY;
        EntityID m_basicAOTextureEntity          = INVALID_ENTITY;

        EntityID m_iconTemplate_DirectionalLightEntity = INVALID_ENTITY;
        EntityID m_iconTemplate_PointLightEntity       = INVALID_ENTITY;
        EntityID m_iconTemplate_SphereLightEntity      = INVALID_ENTITY;

        EntityID m_unitTriangleMeshEntity    = INVALID_ENTITY;
        EntityID m_unitSquareMeshEntity      = INVALID_ENTITY;
        EntityID m_unitPentagonMeshEntity    = INVALID_ENTITY;
        EntityID m_unitHexagonMeshEntity     = INVALID_ENTITY;
        EntityID m_unitTetrahedronMeshEntity = INVALID_ENTITY;
        EntityID m_unitCubeMeshEntity        = INVALID_ENTITY;
        EntityID m_unitOctahedronMeshEntity  = INVALID_ENTITY;
        EntityID m_unitDodecahedronMeshEntity= INVALID_ENTITY;
        EntityID m_unitIcosahedronMeshEntity = INVALID_ENTITY;
        EntityID m_unitSphereMeshEntity      = INVALID_ENTITY;
        EntityID m_terrainMeshEntity         = INVALID_ENTITY;

        EntityID m_defaultMaterialEntity     = INVALID_ENTITY;
    };
}

bool TemplateAssetServiceImpl::LoadTemplateAssets()
{
    ITask::Desc taskDesc("Template Assets Initialization Task", ITask::Type::Once, 2);
    auto l_DefaultAssetInitializationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc,
        [&]() {
            auto l_registry = g_Engine->Get<EntityRegistry>();

            using TexGenFn = void(*)(uint8_t* pixels, uint32_t size);

            auto generateCheckerboard = [](uint8_t* pixels, uint32_t size) {
                uint32_t cellSize = size / 8;
                if (cellSize == 0) cellSize = 1;
                for (uint32_t y = 0; y < size; y++)
                    for (uint32_t x = 0; x < size; x++)
                    {
                        uint32_t idx = (y * size + x) * 4;
                        bool isWhite = ((x / cellSize) + (y / cellSize)) % 2 == 0;
                        uint8_t v = isWhite ? 230 : 180;
                        pixels[idx] = v; pixels[idx+1] = v; pixels[idx+2] = v; pixels[idx+3] = 255;
                    }
            };

            auto generateFlatNormal = [](uint8_t* pixels, uint32_t size) {
                for (uint32_t i = 0; i < size * size * 4; i += 4)
                { pixels[i] = 128; pixels[i+1] = 128; pixels[i+2] = 255; pixels[i+3] = 255; }
            };

            auto generateSolid = [](uint8_t* pixels, uint32_t size, uint8_t v) {
                for (uint32_t i = 0; i < size * size * 4; i += 4)
                { pixels[i] = v; pixels[i+1] = v; pixels[i+2] = v; pixels[i+3] = 255; }
            };

            auto loadOrCreateTexture = [&](const char* name, std::function<void(uint8_t*, uint32_t)> generator, EntityID& entityIDRef) -> bool {
                if (entityIDRef != INVALID_ENTITY)
                    return true;

                auto l_componentName = std::string(name) + "." + TextureComponent::GetTypeName();
                auto l_entityName    = l_componentName;

                auto l_entityID = l_registry->Spawn(ObjectLifespan::Persistence, l_entityName.c_str());
                auto& l_texture = l_registry->Emplace<TextureComponent>(l_entityID);
                l_texture.m_InstanceName = ObjectName(l_componentName.c_str());
                auto* l_texturePtr = &l_texture;

                auto l_filePath = AssetService::GetAssetFilePath(l_componentName.c_str());
                auto l_fullPath = g_Engine->Get<IOService>()->getDataDirectory() + l_filePath;
                std::ifstream l_probe(l_fullPath);
                if (l_probe.good() && (l_probe.close(), AssetService::Load(l_filePath.c_str(), *l_texturePtr, l_entityID)))
                {
                    entityIDRef = l_entityID;
                    return true;
                }

                const uint32_t l_size = 64;
                l_texturePtr->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
                l_texturePtr->m_TextureDesc.Usage = TextureUsage::Sample;
                l_texturePtr->m_TextureDesc.Width = l_size;
                l_texturePtr->m_TextureDesc.Height = l_size;
                l_texturePtr->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
                l_texturePtr->m_TextureDesc.PixelDataType = TexturePixelDataType::UByte;
                l_texturePtr->m_ObjectStatus = ObjectStatus::Created;

                auto* l_pixels = new uint8_t[l_size * l_size * 4];
                generator(l_pixels, l_size);
                m_textureData[l_texturePtr] = l_pixels;

                g_Engine->Get<TextureResourceService>()->Initialize(l_texturePtr, l_pixels, l_entityID);
                entityIDRef = l_entityID;
                return true;
                };

            if (!loadOrCreateTexture("BasicAlbedoTexture",    generateCheckerboard, m_basicAlbedoTextureEntity))    return false;
            if (!loadOrCreateTexture("BasicNormalTexture",     generateFlatNormal,   m_basicNormalTextureEntity))    return false;
            if (!loadOrCreateTexture("BasicMetallicTexture",   [&](uint8_t* p, uint32_t s) { generateSolid(p, s, 0);   }, m_basicMetallicTextureEntity))  return false;
            if (!loadOrCreateTexture("BasicRoughnessTexture",  [&](uint8_t* p, uint32_t s) { generateSolid(p, s, 128); }, m_basicRoughnessTextureEntity)) return false;
            if (!loadOrCreateTexture("BasicAOTexture",         [&](uint8_t* p, uint32_t s) { generateSolid(p, s, 255); }, m_basicAOTextureEntity))        return false;

            if (m_defaultMaterialEntity == INVALID_ENTITY)
            {
                auto l_materialName = std::string("DefaultMaterial.MaterialComponent");
                auto l_entityName   = l_materialName;
                auto l_entityID     = l_registry->Spawn(ObjectLifespan::Persistence, l_entityName.c_str());
                auto& l_material    = l_registry->Emplace<MaterialComponent>(l_entityID);
                l_material.m_InstanceName = ObjectName(l_materialName.c_str());
                auto* l_materialPtr = &l_material;

                auto l_filePath = AssetService::GetAssetFilePath(l_materialName.c_str());
                auto l_fullMatPath = g_Engine->Get<IOService>()->getDataDirectory() + l_filePath;
                std::ifstream l_matProbe(l_fullMatPath);
                if (!(l_matProbe.good() && (l_matProbe.close(), AssetService::Load(l_filePath.c_str(), *l_materialPtr, l_entityID))))
                {
                    // TASK-27: allocation may be recycled; resize() below handles both cases.
                    auto l_matAllocation = AssetService::AllocateMaterialAsset(l_materialName.c_str(), ObjectLifespan::Persistence);
                    l_materialPtr->m_Asset = l_matAllocation.m_Handle;
                    auto* l_matAsset = AssetService::GetMaterialAsset(l_matAllocation.m_Handle);
                    l_matAsset->m_TextureNames.clear();
                    l_matAsset->m_TextureNames.resize(5);
                    l_matAsset->m_TextureNames[0] = l_registry->Get<TextureComponent>(m_basicNormalTextureEntity)->m_InstanceName.c_str();
                    l_matAsset->m_TextureNames[1] = l_registry->Get<TextureComponent>(m_basicAlbedoTextureEntity)->m_InstanceName.c_str();
                    l_matAsset->m_TextureNames[2] = l_registry->Get<TextureComponent>(m_basicMetallicTextureEntity)->m_InstanceName.c_str();
                    l_matAsset->m_TextureNames[3] = l_registry->Get<TextureComponent>(m_basicRoughnessTextureEntity)->m_InstanceName.c_str();
                    l_matAsset->m_TextureNames[4] = l_registry->Get<TextureComponent>(m_basicAOTextureEntity)->m_InstanceName.c_str();
                    l_matAsset->m_ShaderModel = ShaderModel::Opaque;
                    l_matAsset->m_Residency = AssetResidency::Resident;
                    AssetService::Save(*l_materialPtr);

                    g_Engine->Get<MaterialResourceService>()->Initialize(l_materialPtr, l_entityID);
                }
                m_defaultMaterialEntity = l_entityID;
            }

            // Primitive meshes inherit the default material so every TLAS-instanceable
            // entity carries a MaterialComponent. Downstream passes (path tracer, opaque
            // G-buffer) no longer hit the silent white-Lambert fallback on these.
            auto* l_defaultMaterial = l_registry->Get<MaterialComponent>(m_defaultMaterialEntity);
            auto l_defaultMaterialHandle = l_defaultMaterial ? l_defaultMaterial->m_Asset : MaterialAssetHandle{};

            auto loadOrCreateMesh = [&](const char* name, MeshShape shape, EntityID& entityIDRef) {
                if (entityIDRef != INVALID_ENTITY)
                    return;

                auto l_componentName = std::string(name) + ".MeshComponent";
                auto l_entityName    = l_componentName;

                auto l_entityID  = l_registry->Spawn(ObjectLifespan::Persistence, l_entityName.c_str());
                auto& l_mesh     = l_registry->Emplace<MeshComponent>(l_entityID);
                l_mesh.m_InstanceName = ObjectName(l_componentName.c_str());
                auto* l_meshPtr  = &l_mesh;

                auto& l_material = l_registry->Emplace<MaterialComponent>(l_entityID);
                l_material.m_InstanceName = ObjectName((std::string(name) + ".MaterialComponent").c_str());
                l_material.m_Asset = l_defaultMaterialHandle;

                auto l_filePath = AssetService::GetAssetFilePath(l_componentName.c_str());
                if (AssetService::Load(l_filePath.c_str(), *l_meshPtr, l_entityID))
                {
                    entityIDRef = l_entityID;
                    return;
                }

                GenerateMesh(shape, l_meshPtr);
                AssetService::Save(*l_meshPtr, m_meshVertices[l_meshPtr], m_meshIndices[l_meshPtr]);
                g_Engine->Get<MeshResourceService>()->Initialize(l_meshPtr, m_meshVertices[l_meshPtr], m_meshIndices[l_meshPtr], l_entityID);
                entityIDRef = l_entityID;
                };

            loadOrCreateMesh("UnitTriangleMesh",    MeshShape::Triangle,    m_unitTriangleMeshEntity);
            loadOrCreateMesh("UnitSquareMesh",       MeshShape::Square,      m_unitSquareMeshEntity);
            loadOrCreateMesh("UnitPentagonMesh",     MeshShape::Pentagon,    m_unitPentagonMeshEntity);
            loadOrCreateMesh("UnitHexagonMesh",      MeshShape::Hexagon,     m_unitHexagonMeshEntity);
            loadOrCreateMesh("UnitTetrahedronMesh",  MeshShape::Tetrahedron, m_unitTetrahedronMeshEntity);
            loadOrCreateMesh("UnitCubeMesh",         MeshShape::Cube,        m_unitCubeMeshEntity);
            loadOrCreateMesh("UnitOctahedronMesh",   MeshShape::Octahedron,  m_unitOctahedronMeshEntity);
            loadOrCreateMesh("UnitDodecahedronMesh", MeshShape::Dodecahedron,m_unitDodecahedronMeshEntity);
            loadOrCreateMesh("UnitIcosahedronMesh",  MeshShape::Icosahedron, m_unitIcosahedronMeshEntity);
            loadOrCreateMesh("UnitSphereMesh",       MeshShape::Sphere,      m_unitSphereMeshEntity);

            m_terrainMeshEntity = INVALID_ENTITY;

            if (!loadOrCreateTexture("DirectionalLightIcon", [&](uint8_t* p, uint32_t s) { generateSolid(p, s, 255); }, m_iconTemplate_DirectionalLightEntity)) return false;
            if (!loadOrCreateTexture("PointLightIcon",       [&](uint8_t* p, uint32_t s) { generateSolid(p, s, 200); }, m_iconTemplate_PointLightEntity))       return false;
            if (!loadOrCreateTexture("SphereLightIcon",      [&](uint8_t* p, uint32_t s) { generateSolid(p, s, 150); }, m_iconTemplate_SphereLightEntity))      return false;

            return true;
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
            auto l_registry = g_Engine->Get<EntityRegistry>();

            // Template components live in EntityRegistry (not TObjectPool), so
            // Delete() (which calls pool->Destroy) must NOT be called on them.
            // GPU buffers are released by TerminatePool when the graphics service shuts down.
            auto deleteTexture = [&](EntityID& entityIDRef) {
                if (entityIDRef == INVALID_ENTITY) return;
                l_registry->Destroy(entityIDRef);
                entityIDRef = INVALID_ENTITY;
            };

            auto deleteMesh = [&](EntityID& entityIDRef) {
                if (entityIDRef == INVALID_ENTITY) return;
                l_registry->Destroy(entityIDRef);
                entityIDRef = INVALID_ENTITY;
            };

            auto deleteMaterial = [&](EntityID& entityIDRef) {
                if (entityIDRef == INVALID_ENTITY) return;
                l_registry->Destroy(entityIDRef);
                entityIDRef = INVALID_ENTITY;
            };

            deleteTexture(m_basicNormalTextureEntity);
            deleteTexture(m_basicAlbedoTextureEntity);
            deleteTexture(m_basicMetallicTextureEntity);
            deleteTexture(m_basicRoughnessTextureEntity);
            deleteTexture(m_basicAOTextureEntity);

            deleteMaterial(m_defaultMaterialEntity);

            deleteTexture(m_iconTemplate_DirectionalLightEntity);
            deleteTexture(m_iconTemplate_PointLightEntity);
            deleteTexture(m_iconTemplate_SphereLightEntity);

            deleteMesh(m_unitTriangleMeshEntity);
            deleteMesh(m_unitSquareMeshEntity);
            deleteMesh(m_unitPentagonMeshEntity);
            deleteMesh(m_unitHexagonMeshEntity);
            deleteMesh(m_unitTetrahedronMeshEntity);
            deleteMesh(m_unitCubeMeshEntity);
            deleteMesh(m_unitOctahedronMeshEntity);
            deleteMesh(m_unitDodecahedronMeshEntity);
            deleteMesh(m_unitIcosahedronMeshEntity);
            deleteMesh(m_unitSphereMeshEntity);
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
    auto sectorCount = 32;
    auto stackCount = 32;

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

bool TemplateAssetService::Setup(IServiceConfig* systemConfig)
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
    auto l_registry = g_Engine->Get<EntityRegistry>();
    EntityID l_id = INVALID_ENTITY;
    switch (shape)
    {
    case MeshShape::Triangle:    l_id = m_Impl->m_unitTriangleMeshEntity;    break;
    case MeshShape::Square:      l_id = m_Impl->m_unitSquareMeshEntity;      break;
    case MeshShape::Pentagon:    l_id = m_Impl->m_unitPentagonMeshEntity;    break;
    case MeshShape::Hexagon:     l_id = m_Impl->m_unitHexagonMeshEntity;     break;
    case MeshShape::Tetrahedron: l_id = m_Impl->m_unitTetrahedronMeshEntity; break;
    case MeshShape::Cube:        l_id = m_Impl->m_unitCubeMeshEntity;        break;
    case MeshShape::Octahedron:  l_id = m_Impl->m_unitOctahedronMeshEntity;  break;
    case MeshShape::Dodecahedron:l_id = m_Impl->m_unitDodecahedronMeshEntity;break;
    case MeshShape::Icosahedron: l_id = m_Impl->m_unitIcosahedronMeshEntity; break;
    case MeshShape::Sphere:      l_id = m_Impl->m_unitSphereMeshEntity;      break;
    default:
        Log(Error, "Invalid MeshShape!");
        return nullptr;
    }
    return l_registry->Get<MeshComponent>(l_id);
}

TextureComponent* TemplateAssetService::GetTextureComponent(WorldEditorIconType iconType)
{
    auto l_registry = g_Engine->Get<EntityRegistry>();
    switch (iconType)
    {
    case WorldEditorIconType::DIRECTIONAL_LIGHT:
        return l_registry->Get<TextureComponent>(m_Impl->m_iconTemplate_DirectionalLightEntity);
    case WorldEditorIconType::POINT_LIGHT:
        return l_registry->Get<TextureComponent>(m_Impl->m_iconTemplate_PointLightEntity);
    case WorldEditorIconType::SPHERE_LIGHT:
        return l_registry->Get<TextureComponent>(m_Impl->m_iconTemplate_SphereLightEntity);
    default:
        return nullptr;
    }
}

MaterialComponent* TemplateAssetService::GetDefaultMaterialComponent()
{
    return g_Engine->Get<EntityRegistry>()->Get<MaterialComponent>(m_Impl->m_defaultMaterialEntity);
}

bool TemplateAssetService::GenerateMesh(MeshShape shape, MeshComponent* meshComponent)
{
    return m_Impl->GenerateMesh(shape, meshComponent);
}

void TemplateAssetService::FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace)
{
    return m_Impl->FulfillVerticesAndIndices(meshComponent, indices, vertices, verticesPerFace);
}