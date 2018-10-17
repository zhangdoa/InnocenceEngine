#pragma once

#include "../../common/config.h"
#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include <experimental/filesystem>
#endif

#include "../../common/ComponentHeaders.h"

#include "../../common/InnoMath.h"
#include "../../common/InnoType.h"
#include "../../common/InnoAllocator.h"
#include "../../component/AssetSystemSingletonComponent.h"

namespace InnoAssetSystem
{
	__declspec(dllexport) void setup();
	__declspec(dllexport) void initialize();
	__declspec(dllexport) void update();
	__declspec(dllexport) void shutdown();

	__declspec(dllexport) MeshDataComponent* getMesh(meshID meshID);
	__declspec(dllexport) TextureDataComponent* getTexture(textureID textureID);
	__declspec(dllexport) MeshDataComponent* getDefaultMesh(meshShapeType meshShapeType);
	__declspec(dllexport) TextureDataComponent* getDefaultTexture(textureType textureType);
	__declspec(dllexport) void removeMesh(meshID meshID);
	__declspec(dllexport) void removeTexture(textureID textureID);
	__declspec(dllexport) vec4 findMaxVertex(meshID meshID);
	__declspec(dllexport) vec4 findMinVertex(meshID meshID);
	__declspec(dllexport) std::string loadShader(const std::string& fileName);

	__declspec(dllexport) objectStatus getStatus();
};

