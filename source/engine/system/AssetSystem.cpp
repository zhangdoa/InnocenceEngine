#include "AssetSystem.h"
#include "../common/ComponentHeaders.h"
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoAssetSystemNS
{
	void loadAssetsForComponents();

	void assignUnitMesh(MeshShapeType MeshUsageType, VisibleComponent* visibleComponent);

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	std::vector<InnoFuture<void>> m_asyncTask;

	DirectoryMetadata m_rootDirectoryMetadata;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::setup()
{
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::terminate()
{
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::STANDBY;
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem has been terminated.");
	return true;
}

ObjectStatus InnoAssetSystem::getStatus()
{
	return InnoAssetSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT void InnoAssetSystem::loadDefaultAssets()
{
	g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->loadDefaultAssets();
}

INNO_SYSTEM_EXPORT void InnoAssetSystem::loadAssetsForComponents()
{
	InnoAssetSystemNS::loadAssetsForComponents();
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoAssetSystem::addMeshDataComponent()
{
	return nullptr;
}

INNO_SYSTEM_EXPORT MaterialDataComponent * InnoAssetSystem::addMaterialDataComponent()
{
	return nullptr;
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::addTextureDataComponent()
{
	return nullptr;
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoAssetSystem::getMeshDataComponent(EntityID meshID)
{
	return nullptr;
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(EntityID textureID)
{
	return nullptr;
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoAssetSystem::getMeshDataComponent(MeshShapeType MeshShapeType)
{
	return nullptr;
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(TextureUsageType TextureUsageType)
{
	return nullptr;
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(FileExplorerIconType iconType)
{
	return nullptr;
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoAssetSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	return nullptr;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::removeMeshDataComponent(EntityID EntityID)
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::removeTextureDataComponent(EntityID EntityID)
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::releaseRawDataForMeshDataComponent(EntityID EntityID)
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoAssetSystem::releaseRawDataForTextureDataComponent(EntityID EntityID)
{
	return true;
}

INNO_SYSTEM_EXPORT DirectoryMetadata* InnoAssetSystem::getRootDirectoryMetadata()
{
	return &InnoAssetSystemNS::m_rootDirectoryMetadata;
}

void InnoAssetSystem::addUnitCube(MeshDataComponent& meshDataComponent)
{
	float vertices[] = {
		// positions     // normals      // texture coords
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

		-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
	};

	meshDataComponent.m_vertices.reserve(288);

	for (size_t i = 0; i < 288; i += 8)
	{
		meshDataComponent.m_vertices.emplace_back(
			Vertex(
				vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0f),
				vec2(vertices[i + 6], vertices[i + 7]),
				vec4(vertices[i + 3], vertices[i + 4], vertices[i + 5], 0.0f)
			)
		);
	}

	meshDataComponent.m_indices.reserve(36);

	for (unsigned int i = 0; i < 36; i++)
	{
		meshDataComponent.m_indices.emplace_back(i);
	}

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addUnitSphere(MeshDataComponent& meshDataComponent)
{
	unsigned int X_SEGMENTS = 64;
	unsigned int Y_SEGMENTS = 64;
	auto l_containerSize = X_SEGMENTS * Y_SEGMENTS;
	meshDataComponent.m_vertices.reserve(l_containerSize);

	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = cos(xSegment * 2.0f * PI<float>) * sin(ySegment * PI<float>);
			float yPos = cos(ySegment * PI<float>);
			float zPos = sin(xSegment * 2.0f * PI<float>) * sin(ySegment * PI<float>);

			Vertex l_VertexData;
			l_VertexData.m_pos = vec4(xPos, yPos, zPos, 1.0f);
			l_VertexData.m_texCoord = vec2(xSegment, ySegment);
			l_VertexData.m_normal = vec4(xPos, yPos, zPos, 0.0f).normalize();
			meshDataComponent.m_vertices.emplace_back(l_VertexData);
		}
	}

	bool oddRow = false;
	for (unsigned y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned x = 0; x <= X_SEGMENTS; ++x)
			{
				meshDataComponent.m_indices.push_back(y    * (X_SEGMENTS + 1) + x);
				meshDataComponent.m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				meshDataComponent.m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				meshDataComponent.m_indices.push_back(y    * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
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
						l_visibleComponent->m_objectStatus = ObjectStatus::ALIVE;
					});
				}
			}
			else
			{
				g_pCoreSystem->getTaskSystem()->submit([=]()
				{
					assignUnitMesh(l_visibleComponent->m_meshShapeType, l_visibleComponent);
					g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(l_visibleComponent);
					l_visibleComponent->m_objectStatus = ObjectStatus::ALIVE;
				});
			}
		}
		g_pCoreSystem->getTaskSystem()->shrinkFutureContainer(InnoAssetSystemNS::m_asyncTask);
	}
}

void InnoAssetSystemNS::assignUnitMesh(MeshShapeType meshUsageType, VisibleComponent* visibleComponent)
{
	if (meshUsageType == MeshShapeType::CUSTOM)
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