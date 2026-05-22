#pragma once
#include "TemplateAssetService.h"
#include "../Common/HashMap.h"

#include "EntityRegistry.h"

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

        Inno::HashMap<MeshComponent*, std::vector<Vertex>> m_meshVertices;
        Inno::HashMap<MeshComponent*, std::vector<Index>> m_meshIndices;
        Inno::HashMap<TextureComponent*, void*> m_textureData;

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
