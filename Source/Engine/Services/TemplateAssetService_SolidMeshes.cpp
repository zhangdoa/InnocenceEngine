#include "TemplateAssetService.h"
#include "../Common/Array.h"
#include "TemplateAssetService_Internal.h"
using namespace Inno;

void TemplateAssetServiceImpl::addTetrahedron(MeshComponent* meshComponent)
{
    Inno::Array<Index> l_indices =
    {
        0, 3, 1, 0, 2, 3,
        0, 1, 2, 1, 3, 2
    };

    Inno::Array<Vec3> l_vertices =
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
    Inno::Array<Index> l_indices =
    {
        0, 3, 1, 1, 3, 2,
        4, 0, 5, 5, 0, 1,
        7, 4, 6, 6, 4, 5,
        3, 7, 2, 2, 7, 6,
        4, 7, 0, 0, 7, 3,
        1, 2, 5, 5, 2, 6
    };

    Inno::Array<Vec3> l_vertices =
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
    Inno::Array<Index> l_indices =
    {
        0, 2, 4, 4, 2, 1,
        1, 2, 5, 5, 2, 0,
        0, 4, 3, 4, 1, 3,
        1, 5, 3, 5, 0, 3
    };

    Inno::Array<Vec3> l_vertices =
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
    Inno::Array<Index> l_indices =
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

    Inno::Array<Vec3> l_vertices =
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
