#include "AssetSystem.h"
#include "../Common/ComponentHeaders.h"
#include "../ModuleManager/IModuleManager.h"
#include "../Common/InnoMathHelper.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE InnoAssetSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	DirectoryMetadata m_rootDirectoryMetadata;
}

bool InnoAssetSystem::setup()
{
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoAssetSystem::initialize()
{
	if (InnoAssetSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoAssetSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem has been initialized.");
		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: Object is not created!");
		return false;
	}
}

bool InnoAssetSystem::update()
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

bool InnoAssetSystem::terminate()
{
	InnoAssetSystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem has been terminated.");
	return true;
}

ObjectStatus InnoAssetSystem::getStatus()
{
	return InnoAssetSystemNS::m_objectStatus;
}

DirectoryMetadata* InnoAssetSystem::getRootDirectoryMetadata()
{
	return &InnoAssetSystemNS::m_rootDirectoryMetadata;
}

void InnoAssetSystem::addUnitCube(MeshDataComponent& meshDataComponent)
{
	meshDataComponent.m_vertices.reserve(8);
	InnoMath::generateNDC(&meshDataComponent.m_vertices[0]);

	std::vector<Index> l_indices =
	{
		0, 3, 1, 1, 3, 2,
		4, 0, 5, 5, 0, 1,
		7, 4, 6, 6, 4, 5,
		3, 7, 2, 2, 7, 6,
		7, 0, 4, 0, 7, 3,
		1, 2, 5, 5, 2, 6
	};

	meshDataComponent.m_indices.reserve(36);

	for (unsigned int i = 0; i < 36; i++)
	{
		meshDataComponent.m_indices[i] = l_indices[i];
	}

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

	meshDataComponent.m_vertices.reserve((stackCount + 1) * (sectorCount + 1));

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

	meshDataComponent.m_indices.reserve(stackCount * (sectorCount - 1) * 6);

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
	meshDataComponent.m_vertices.reserve(4);

	meshDataComponent.m_vertices[0].m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[0].m_texCoord = vec2(1.0f, 1.0f);

	meshDataComponent.m_vertices[1].m_pos = vec4(1.0f, -1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[1].m_texCoord = vec2(1.0f, 0.0f);

	meshDataComponent.m_vertices[2].m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[2].m_texCoord = vec2(0.0f, 0.0f);

	meshDataComponent.m_vertices[3].m_pos = vec4(-1.0f, 1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[3].m_texCoord = vec2(0.0f, 1.0f);

	meshDataComponent.m_indices.reserve(6);
	meshDataComponent.m_indices[0] = 0;
	meshDataComponent.m_indices[1] = 1;
	meshDataComponent.m_indices[2] = 3;
	meshDataComponent.m_indices[3] = 1;
	meshDataComponent.m_indices[4] = 2;
	meshDataComponent.m_indices[5] = 3;

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void InnoAssetSystem::addUnitLine(MeshDataComponent& meshDataComponent)
{
	meshDataComponent.m_vertices.reserve(2);

	meshDataComponent.m_vertices[0].m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[0].m_texCoord = vec2(1.0f, 1.0f);

	meshDataComponent.m_vertices[1].m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	meshDataComponent.m_vertices[1].m_texCoord = vec2(0.0f, 0.0f);

	meshDataComponent.m_indices.reserve(2);
	meshDataComponent.m_indices[0] = 0;
	meshDataComponent.m_indices[1] = 1;

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

TextureDataComponent* InnoAssetSystem::loadTexture(const std::string& fileName, TextureSamplerType samplerType, TextureUsageType usageType)
{
	auto l_TDC = g_pModuleManager->getFileSystem()->loadTexture(fileName);
	l_TDC->m_textureDataDesc.samplerType = samplerType;
	l_TDC->m_textureDataDesc.usageType = usageType;
	return l_TDC;
}