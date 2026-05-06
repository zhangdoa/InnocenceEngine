#include "TemplateAssetService.h"
#include "TemplateAssetService_Internal.h"

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

            // Every primitive mesh entity carries the default MaterialComponent so the
            // "entity-with-mesh has a material" invariant holds by construction.
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
