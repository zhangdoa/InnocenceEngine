#pragma once
#include "ISystem.h"

#include "../Component/MeshComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/SkeletonComponent.h"
#include "../Component/AnimationComponent.h"
#include "../Component/VisibleComponent.h"

namespace Inno
{
	class IAssetSystem : public IComponentSystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IAssetSystem);

		virtual bool convertModel(const char* fileName, const char* exportPath) = 0;
		virtual Model* loadModel(const char* fileName, bool AsyncUploadGPUResource = true) = 0;
		virtual TextureComponent* loadTexture(const char* fileName) = 0;
		virtual bool saveTexture(const char* fileName, TextureComponent* TextureComp) = 0;

		virtual bool loadAssetsForComponents(bool AsyncLoad = true) = 0;

		virtual bool recordLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair* pair) = 0;
		virtual bool findLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair*& pair) = 0;

		virtual bool recordLoadedModel(const char* fileName, Model* model) = 0;
		virtual bool findLoadedModel(const char* fileName, Model*& model) = 0;

		virtual bool recordLoadedTexture(const char* fileName, TextureComponent* texture) = 0;
		virtual bool findLoadedTexture(const char* fileName, TextureComponent*& texture) = 0;

		virtual bool recordLoadedSkeleton(const char* fileName, SkeletonComponent* skeleton) = 0;
		virtual bool findLoadedSkeleton(const char* fileName, SkeletonComponent*& skeleton) = 0;

		virtual bool recordLoadedAnimation(const char* fileName, AnimationComponent* animation) = 0;
		virtual bool findLoadedAnimation(const char* fileName, AnimationComponent*& animation) = 0;

		virtual ArrayRangeInfo addMeshMaterialPairs(uint64_t count) = 0;
		virtual MeshMaterialPair* getMeshMaterialPair(uint64_t index) = 0;

		virtual Model* addModel() = 0;

		virtual Model* addProceduralModel(ProceduralMeshShape shape, ShaderModel shaderModel = ShaderModel::Opaque) = 0;
		virtual bool generateProceduralMesh(ProceduralMeshShape shape, MeshComponent* meshComponent) = 0;
	};
}