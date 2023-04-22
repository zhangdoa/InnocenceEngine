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

		virtual bool ConvertModel(const char* fileName, const char* exportPath) = 0;
		virtual Model* LoadModel(const char* fileName, bool AsyncUploadGPUResource = true) = 0;
		virtual TextureComponent* LoadTexture(const char* fileName) = 0;
		virtual bool SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData) = 0;		
		virtual bool SaveTexture(const char* fileName, TextureComponent* TextureComp) = 0;

		virtual bool LoadAssetsForComponents(bool AsyncLoad = true) = 0;

		virtual bool RecordLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair* pair) = 0;
		virtual bool FindLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair*& pair) = 0;

		virtual bool RecordLoadedModel(const char* fileName, Model* model) = 0;
		virtual bool FindLoadedModel(const char* fileName, Model*& model) = 0;

		virtual bool RecordLoadedTexture(const char* fileName, TextureComponent* texture) = 0;
		virtual bool FindLoadedTexture(const char* fileName, TextureComponent*& texture) = 0;

		virtual bool RecordLoadedSkeleton(const char* fileName, SkeletonComponent* skeleton) = 0;
		virtual bool FindLoadedSkeleton(const char* fileName, SkeletonComponent*& skeleton) = 0;

		virtual bool RecordLoadedAnimation(const char* fileName, AnimationComponent* animation) = 0;
		virtual bool FindLoadedAnimation(const char* fileName, AnimationComponent*& animation) = 0;

		virtual ArrayRangeInfo AddMeshMaterialPairs(uint64_t count) = 0;
		virtual MeshMaterialPair* GetMeshMaterialPair(uint64_t index) = 0;

		virtual Model* AddModel() = 0;

		virtual Model* AddProceduralModel(ProceduralMeshShape shape, ShaderModel shaderModel = ShaderModel::Opaque) = 0;
		virtual bool GenerateProceduralMesh(ProceduralMeshShape shape, MeshComponent* meshComponent) = 0;
		virtual void FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace) = 0;
	};
}