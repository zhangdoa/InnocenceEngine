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
	InnoHighLevelSystem_EXPORT bool setup();
	InnoHighLevelSystem_EXPORT bool initialize();
	InnoHighLevelSystem_EXPORT bool update();
	InnoHighLevelSystem_EXPORT bool terminate();

	MeshDataComponent* getMesh(meshID meshID);
	TextureDataComponent* getTexture(textureID textureID);
	MeshDataComponent* getDefaultMesh(meshShapeType meshShapeType);
	TextureDataComponent* getDefaultTexture(textureType textureType);
	void removeMesh(meshID meshID);
	void removeTexture(textureID textureID);
	vec4 findMaxVertex(meshID meshID);
	vec4 findMinVertex(meshID meshID);
	std::string loadShader(const std::string& fileName);

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};

