#include "TemplateAssetService.h"
#include "../Common/Array.h"
#include "TemplateAssetService_Internal.h"

#include "EntityRegistry.h"
#include "../Engine.h"
using namespace Inno;

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

void TemplateAssetService::FulfillVerticesAndIndices(MeshComponent* meshComponent, const Inno::Array<Index>& indices, const Inno::Array<Vec3>& vertices, uint32_t verticesPerFace)
{
    return m_Impl->FulfillVerticesAndIndices(meshComponent, indices, vertices, verticesPerFace);
}
