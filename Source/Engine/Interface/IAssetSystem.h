#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"
#include "../Component/MeshDataComponent.h"
#include "../Component/MaterialDataComponent.h"
#include "../Component/TextureDataComponent.h"
#include "../Component/SkeletonDataComponent.h"
#include "../Component/AnimationDataComponent.h"

#include "../Component/VisibleComponent.h"

class IAssetSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IAssetSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool convertModel(const char* fileName, const char* exportPath) = 0;
	virtual Model* loadModel(const char* fileName, bool AsyncUploadGPUResource = true) = 0;
	virtual TextureDataComponent* loadTexture(const char* fileName) = 0;
	virtual bool saveTexture(const char* fileName, TextureDataComponent* TDC) = 0;

	virtual bool loadAssetsForComponents(bool AsyncLoad = true) = 0;

	virtual bool recordLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair* pair) = 0;
	virtual bool findLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair*& pair) = 0;

	virtual bool recordLoadedModel(const char* fileName, Model* model) = 0;
	virtual bool findLoadedModel(const char* fileName, Model*& model) = 0;

	virtual bool recordLoadedTexture(const char* fileName, TextureDataComponent* texture) = 0;
	virtual bool findLoadedTexture(const char* fileName, TextureDataComponent*& texture) = 0;

	virtual bool recordLoadedSkeleton(const char* fileName, SkeletonDataComponent* skeleton) = 0;
	virtual bool findLoadedSkeleton(const char* fileName, SkeletonDataComponent*& skeleton) = 0;

	virtual bool recordLoadedAnimation(const char* fileName, AnimationDataComponent* animation) = 0;
	virtual bool findLoadedAnimation(const char* fileName, AnimationDataComponent*& animation) = 0;

	virtual ArrayRangeInfo addMeshMaterialPairs(uint64_t count) = 0;
	virtual MeshMaterialPair* getMeshMaterialPair(uint64_t index) = 0;

	virtual Model* addModel() = 0;

	virtual Model* addProceduralModel(ProceduralMeshShape shape, ShaderModel shaderModel = ShaderModel::Opaque) = 0;
	virtual bool generateProceduralMesh(ProceduralMeshShape shape, MeshDataComponent* meshDataComponent) = 0;
};
