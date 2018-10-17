#pragma once
#include "../../common/config.h"
#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include <experimental/filesystem>
#endif

#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/ComponentHeaders.h"

#include "../../common/InnoMath.h"
#include "../../common/InnoType.h"
#include "../../common/InnoAllocator.h"
#include "../../component/AssetSystemSingletonComponent.h"

namespace InnoAssetSystem
{
	InnoHighLevelSystem_EXPORT void setup();
	InnoHighLevelSystem_EXPORT void initialize();
	InnoHighLevelSystem_EXPORT void update();
	InnoHighLevelSystem_EXPORT void shutdown();

	InnoHighLevelSystem_EXPORT MeshDataComponent* getMesh(meshID meshID);
	InnoHighLevelSystem_EXPORT TextureDataComponent* getTexture(textureID textureID);
	InnoHighLevelSystem_EXPORT MeshDataComponent* getDefaultMesh(meshShapeType meshShapeType);
	InnoHighLevelSystem_EXPORT TextureDataComponent* getDefaultTexture(textureType textureType);
	InnoHighLevelSystem_EXPORT void removeMesh(meshID meshID);
	InnoHighLevelSystem_EXPORT void removeTexture(textureID textureID);
	InnoHighLevelSystem_EXPORT vec4 findMaxVertex(meshID meshID);
	InnoHighLevelSystem_EXPORT vec4 findMinVertex(meshID meshID);
	InnoHighLevelSystem_EXPORT std::string loadShader(const std::string& fileName);

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};

