#pragma once
#include "../../common/config.h"
#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include <experimental/filesystem>
#endif

#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/ComponentHeaders.h"

#include "../../common/InnoMath.h"
#include "../../common/InnoType.h"
#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"

namespace InnoAssetSystem
{
	InnoHighLevelSystem_EXPORT bool setup();
	InnoHighLevelSystem_EXPORT bool initialize();
	InnoHighLevelSystem_EXPORT bool update();
	InnoHighLevelSystem_EXPORT bool terminate();

	MeshDataComponent* getMeshDataComponent(EntityID meshID);
	TextureDataComponent* getTextureDataComponent(EntityID textureID);
	MeshDataComponent* getDefaultMeshDataComponent(meshShapeType meshShapeType);
	TextureDataComponent* getDefaultTextureDataComponent(textureType textureType);
	void removeMeshDataComponent(EntityID EntityID);
	void removeTextureDataComponent(EntityID EntityID);
	void releaseRawDataForMeshDataComponent(EntityID EntityID);
	void releaseRawDataForTextureDataComponent(EntityID EntityID);
	vec4 findMaxVertex(EntityID meshID);
	vec4 findMinVertex(EntityID meshID);
	std::string loadShader(const std::string& fileName);

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};

