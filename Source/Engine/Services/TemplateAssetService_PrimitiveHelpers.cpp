#include "TemplateAssetService.h"
#include "../Common/Array.h"
#include "TemplateAssetService_Internal.h"
using namespace Inno;

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

void TemplateAssetServiceImpl::FulfillVerticesAndIndices(MeshComponent* meshComponent, const Inno::Array<Index>& indices, const Inno::Array<Vec3>& vertices, uint32_t verticesPerFace)
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
