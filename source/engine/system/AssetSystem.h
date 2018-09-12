#pragma once

#include "../common/config.h"
#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include <experimental/filesystem>
#endif

#include "../common/ComponentHeaders.h"

#include "../common/InnoMath.h"
#include "../common/InnoType.h"
#include "../common/InnoAllocator.h"
#include "../component/AssetSystemSingletonComponent.h"

namespace InnoAssetSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	meshID addMesh();
	meshID addMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
	textureID addTexture(textureType textureType);
	MeshDataComponent* getMesh(meshID meshID);
	TextureDataComponent* getTexture(textureID textureID);
	MeshDataComponent* getDefaultMesh(meshShapeType meshShapeType);
	TextureDataComponent* getDefaultTexture(textureType textureType);
	void removeMesh(meshID meshID);
	void removeTexture(textureID textureID);
	vec4 findMaxVertex(meshID meshID);
	vec4 findMinVertex(meshID meshID);
	std::string loadShader(const std::string& fileName);

	objectStatus m_AssetSystemStatus = objectStatus::SHUTDOWN;
	AssetSystemSingletonComponent* m_AssetSystemSingletonComponent;
};

