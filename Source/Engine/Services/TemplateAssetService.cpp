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
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Triangle, m_unitTriangleMesh);
	m_unitTriangleMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitTriangleMesh->m_ProceduralMeshShape = ProceduralMeshShape::Triangle;
	m_unitTriangleMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitSquareMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitSquareMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Square, m_unitSquareMesh);
	m_unitSquareMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitSquareMesh->m_ProceduralMeshShape = ProceduralMeshShape::Square;
	m_unitSquareMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitPentagonMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitPentagonMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Pentagon, m_unitPentagonMesh);
	m_unitPentagonMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitPentagonMesh->m_ProceduralMeshShape = ProceduralMeshShape::Pentagon;
	m_unitPentagonMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitHexagonMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitHexagonMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Hexagon, m_unitHexagonMesh);
	m_unitHexagonMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitHexagonMesh->m_ProceduralMeshShape = ProceduralMeshShape::Hexagon;
	m_unitHexagonMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitTetrahedronMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitTetrahedronMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Tetrahedron, m_unitTetrahedronMesh);
	m_unitTetrahedronMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitTetrahedronMesh->m_ProceduralMeshShape = ProceduralMeshShape::Tetrahedron;
	m_unitTetrahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitCubeMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitCubeMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Cube, m_unitCubeMesh);
	m_unitCubeMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitCubeMesh->m_ProceduralMeshShape = ProceduralMeshShape::Cube;
	m_unitCubeMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitOctahedronMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitOctahedronMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Octahedron, m_unitOctahedronMesh);
	m_unitOctahedronMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitOctahedronMesh->m_ProceduralMeshShape = ProceduralMeshShape::Octahedron;
	m_unitOctahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitDodecahedronMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitDodecahedronMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Dodecahedron, m_unitDodecahedronMesh);
	m_unitDodecahedronMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitDodecahedronMesh->m_ProceduralMeshShape = ProceduralMeshShape::Dodecahedron;
	m_unitDodecahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitIcosahedronMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitIcosahedronMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Icosahedron, m_unitIcosahedronMesh);
	m_unitIcosahedronMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitIcosahedronMesh->m_ProceduralMeshShape = ProceduralMeshShape::Icosahedron;
	m_unitIcosahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitSphereMesh = g_Engine->getRenderingServer()->AddMeshComponent("UnitSphereMesh/");
	g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Sphere, m_unitSphereMesh);
	m_unitSphereMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitSphereMesh->m_ProceduralMeshShape = ProceduralMeshShape::Sphere;
	m_unitSphereMesh->m_ObjectStatus = ObjectStatus::Created;

	ITask::Desc taskDesc;
	taskDesc.m_Name = "Default Assets Initialization Task";
	taskDesc.m_Type = ITask::Type::Once;
	taskDesc.m_ThreadID = 2;

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

MeshComponent* TemplateAssetService::GetMeshComponent(ProceduralMeshShape shape)
{
	switch (shape)
	{
	case ProceduralMeshShape::Triangle:
		return m_Impl->m_unitTriangleMesh;
		break;
	case ProceduralMeshShape::Square:
		return m_Impl->m_unitSquareMesh;
		break;
	case ProceduralMeshShape::Pentagon:
		return m_Impl->m_unitPentagonMesh;
		break;
	case ProceduralMeshShape::Hexagon:
		return m_Impl->m_unitHexagonMesh;
		break;
	case ProceduralMeshShape::Tetrahedron:
		return m_Impl->m_unitTetrahedronMesh;
		break;
	case ProceduralMeshShape::Cube:
		return m_Impl->m_unitCubeMesh;
		break;
	case ProceduralMeshShape::Octahedron:
		return m_Impl->m_unitOctahedronMesh;
		break;
	case ProceduralMeshShape::Dodecahedron:
		return m_Impl->m_unitDodecahedronMesh;
		break;
	case ProceduralMeshShape::Icosahedron:
		return m_Impl->m_unitIcosahedronMesh;
		break;
	case ProceduralMeshShape::Sphere:
		return m_Impl->m_unitSphereMesh;
		break;
	default:
		Log(Error, "Invalid ProceduralMeshShape!");
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
