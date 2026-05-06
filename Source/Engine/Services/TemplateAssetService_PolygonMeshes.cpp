#include "TemplateAssetService.h"
#include "TemplateAssetService_Internal.h"
using namespace Inno;

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
