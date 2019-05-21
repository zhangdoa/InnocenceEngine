#include "AssetSystem.h"
#include "../common/ComponentHeaders.h"
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoAssetSystemNS
{
	void loadAssetsForComponents();

	void assignUnitMesh(MeshShapeType MeshUsageType, VisibleComponent* visibleComponent);

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	std::vector<InnoFuture<void>> m_asyncTask;

	DirectoryMetadata m_rootDirectoryMetadata;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::setup()
{
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::initialize()
{
	if (InnoAssetSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoAssetSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem has been initialized.");
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: Object is not created!");
		return false;
	}
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::update()
{
	if (InnoAssetSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		InnoAssetSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::terminate()
{
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem has been terminated.");
	return true;
}

ObjectStatus InnoAssetSystem::getStatus()
{
	return InnoAssetSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT void InnoAssetSystem::loadAssetsForComponents()
{
	InnoAssetSystemNS::loadAssetsForComponents();
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoAssetSystem::addMeshDataComponent()
{
	return g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->addMeshDataComponent();
}

INNO_SYSTEM_EXPORT MaterialDataComponent * InnoAssetSystem::addMaterialDataComponent()
{
	return g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->addMaterialDataComponent();
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::addTextureDataComponent()
{
	return g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->addTextureDataComponent();
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoAssetSystem::getMeshDataComponent(MeshShapeType meshShapeType)
{
	return g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getMeshDataComponent(meshShapeType);
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(TextureUsageType textureUsageType)
{
	return g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getTextureDataComponent(textureUsageType);
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(FileExplorerIconType iconType)
{
	return g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getTextureDataComponent(iconType);
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	return g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getTextureDataComponent(iconType);
}

INNO_SYSTEM_EXPORT DirectoryMetadata* InnoAssetSystem::getRootDirectoryMetadata()
{
	return &InnoAssetSystemNS::m_rootDirectoryMetadata;
}

void InnoAssetSystem::addUnitCube(MeshDataComponent& meshDataComponent)
{
	auto l_NDC = InnoMath::generateNDC<float>();
	meshDataComponent.m_vertices = l_NDC;

	std::vector<Index> l_indices =
	{
		0, 3, 1, 1, 3, 2,
		4, 0, 5, 5, 0, 1,
		7, 4, 6, 6, 4, 5,
		3, 7, 2, 2, 7, 6,
		7, 0, 4, 0, 7, 3,
		1, 2, 5, 5, 2, 6
	};

	meshDataComponent.m_indices = l_indices;
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addUnitSphere(MeshDataComponent& meshDataComponent)
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

	for (int i = 0; i <= stackCount; ++i)
	{
		stackAngle = PI<float> / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = radius * cosf(stackAngle);             // r * cos(u)
		z = radius * sinf(stackAngle);              // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// the first and last vertices have same position and normal, but different tex coords
		for (int j = 0; j <= sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			Vertex l_VertexData;
			l_VertexData.m_pos = vec4(x, y, z, 1.0f);

			// normalized vertex normal (nx, ny, nz)
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			l_VertexData.m_normal = vec4(nx, ny, nz, 1.0f);

			// vertex tex coord (s, t) range between [0, 1]
			s = (float)j / sectorCount;
			t = (float)i / stackCount;
			l_VertexData.m_texCoord = vec2(s, t);

			meshDataComponent.m_vertices.emplace_back(l_VertexData);
		}
	}

	int k1, k2;
	for (int i = 0; i < stackCount; ++i)
	{
		k1 = i * (sectorCount + 1);     // beginning of current stack
		k2 = k1 + sectorCount + 1;      // beginning of next stack

		for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				meshDataComponent.m_indices.emplace_back(k1);
				meshDataComponent.m_indices.emplace_back(k2);
				meshDataComponent.m_indices.emplace_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				meshDataComponent.m_indices.emplace_back(k1 + 1);
				meshDataComponent.m_indices.emplace_back(k2);
				meshDataComponent.m_indices.emplace_back(k2 + 1);
			}
		}
	}

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addUnitQuad(MeshDataComponent& meshDataComponent)
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4 };
	meshDataComponent.m_indices = { 0, 1, 3, 1, 2, 3 };
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addUnitLine(MeshDataComponent& meshDataComponent)
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(0.0f, 0.0f);

	meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2 };
	meshDataComponent.m_indices = { 0, 1 };
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addTerrain(MeshDataComponent& meshDataComponent)
{
	auto l_gridSize = 256;
	auto l_gridSize2 = l_gridSize * l_gridSize;
	auto l_gridSizehalf = l_gridSize / 2;
	meshDataComponent.m_vertices.reserve(l_gridSize2 * 4);
	meshDataComponent.m_indices.reserve(l_gridSize2 * 6);

	for (auto j = 0; j < l_gridSize; j++)
	{
		for (auto i = 0; i < l_gridSize; i++)
		{
			auto l_px0 = (float)(i - l_gridSizehalf);
			auto l_px1 = l_px0 + 1.0f;
			auto l_pz0 = (float)(j - l_gridSizehalf);
			auto l_pz1 = l_pz0 + 1.0f;

			auto l_tx0 = l_px0 / (float)l_gridSize;
			auto l_tx1 = l_px1 / (float)l_gridSize;
			auto l_tz0 = l_pz0 / (float)l_gridSize;
			auto l_tz1 = l_pz1 / (float)l_gridSize;

			Vertex l_VertexData_1;
			l_VertexData_1.m_pos = vec4(l_px0, 0.0f, l_pz0, 1.0f);
			l_VertexData_1.m_texCoord = vec2(l_tx0, l_tz0);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_1);

			Vertex l_VertexData_2;
			l_VertexData_2.m_pos = vec4(l_px0, 0.0f, l_pz1, 1.0f);
			l_VertexData_2.m_texCoord = vec2(l_tx0, l_tz1);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_2);

			Vertex l_VertexData_3;
			l_VertexData_3.m_pos = vec4(l_px1, 0.0f, l_pz1, 1.0f);
			l_VertexData_3.m_texCoord = vec2(l_tx1, l_tz1);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_3);

			Vertex l_VertexData_4;
			l_VertexData_4.m_pos = vec4(l_px1, 0.0f, l_pz0, 1.0f);
			l_VertexData_4.m_texCoord = vec2(l_tx1, l_tz0);
			meshDataComponent.m_vertices.emplace_back(l_VertexData_4);

			auto l_gridIndex = 4 * (i)+4 * l_gridSize * (j);
			meshDataComponent.m_indices.emplace_back(0 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(1 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(3 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(1 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(2 + l_gridIndex);
			meshDataComponent.m_indices.emplace_back(3 + l_gridIndex);
		}
	}
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystemNS::loadAssetsForComponents()
{
	for (auto l_environmentCaptureComponent : g_pCoreSystem->getGameSystem()->get<EnvironmentCaptureComponent>())
	{
		if (!l_environmentCaptureComponent->m_cubemapTextureFileName.empty())
		{
			//InnoAssetSystemNS::m_asyncTask.emplace_back(g_pCoreSystem->getTaskSystem()->submit([&]()
			//{
			//	l_environmentCaptureComponent->m_TDC = InnoAssetSystemNS::loadTexture(l_environmentCaptureComponent->m_cubemapTextureFileName, TextureUsageType::EQUIRETANGULAR);
			//}));
		}
	}

	for (auto l_visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (l_visibleComponent->m_visiblilityType != VisiblilityType::INNO_INVISIBLE)
		{
			if (l_visibleComponent->m_meshShapeType == MeshShapeType::CUSTOM)
			{
				if (!l_visibleComponent->m_modelFileName.empty())
				{
					g_pCoreSystem->getTaskSystem()->submit([=]()
					{
						l_visibleComponent->m_modelMap = g_pCoreSystem->getAssetSystem()->loadModel(l_visibleComponent->m_modelFileName);
						g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(l_visibleComponent);
						l_visibleComponent->m_objectStatus = ObjectStatus::Activated;
					});
				}
			}
			else
			{
				g_pCoreSystem->getTaskSystem()->submit([=]()
				{
					assignUnitMesh(l_visibleComponent->m_meshShapeType, l_visibleComponent);
					g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(l_visibleComponent);
					l_visibleComponent->m_objectStatus = ObjectStatus::Activated;
				});
			}
		}
		g_pCoreSystem->getTaskSystem()->shrinkFutureContainer(InnoAssetSystemNS::m_asyncTask);
	}
}

void InnoAssetSystemNS::assignUnitMesh(MeshShapeType meshUsageType, VisibleComponent* visibleComponent)
{
	if (meshUsageType != MeshShapeType::CUSTOM)
	{
		MeshDataComponent* l_UnitMeshTemplate = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getMeshDataComponent(meshUsageType);
		visibleComponent->m_modelMap.emplace(l_UnitMeshTemplate, g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->addMaterialDataComponent());
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: don't assign unit mesh to a custom mesh shape component!");
	}
}

ModelMap InnoAssetSystem::loadModel(const std::string & fileName)
{
	auto l_result = g_pCoreSystem->getFileSystem()->loadModel(fileName);
	return l_result;
}

TextureDataComponent* InnoAssetSystem::loadTexture(const std::string& fileName, TextureSamplerType samplerType, TextureUsageType usageType)
{
	auto l_TDC = g_pCoreSystem->getFileSystem()->loadTexture(fileName);
	l_TDC->m_textureDataDesc.samplerType = samplerType;
	l_TDC->m_textureDataDesc.usageType = usageType;
	return l_TDC;
}