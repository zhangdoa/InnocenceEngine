#pragma once
#include "../Interface/IAssetSystem.h"

namespace Inno
{
	class AssetSystem : public IAssetSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AssetSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool convertModel(const char* fileName, const char* exportPath) override;
		Model* loadModel(const char* fileName, bool AsyncUploadGPUResource = true) override;
		TextureComponent* loadTexture(const char* fileName) override;
		bool saveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData) override;
		bool saveTexture(const char* fileName, TextureComponent* TextureComp) override;

		bool loadAssetsForComponents(bool AsyncLoad) override;

		bool recordLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair* pair) override;
		bool findLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair*& pair) override;

		bool recordLoadedModel(const char* fileName, Model* model) override;
		bool findLoadedModel(const char* fileName, Model*& model) override;

		bool recordLoadedTexture(const char* fileName, TextureComponent* texture) override;
		bool findLoadedTexture(const char* fileName, TextureComponent*& texture) override;

		bool recordLoadedSkeleton(const char* fileName, SkeletonComponent* skeleton) override;
		bool findLoadedSkeleton(const char* fileName, SkeletonComponent*& skeleton) override;

		bool recordLoadedAnimation(const char* fileName, AnimationComponent* animation) override;
		bool findLoadedAnimation(const char* fileName, AnimationComponent*& animation) override;

		ArrayRangeInfo addMeshMaterialPairs(uint64_t count) override;
		MeshMaterialPair* getMeshMaterialPair(uint64_t index) override;

		Model* addModel() override;

		Model* addProceduralModel(ProceduralMeshShape shape, ShaderModel shaderModel) override;
		bool generateProceduralMesh(ProceduralMeshShape shape, MeshComponent* meshComponent) override;
		void fulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace) override;
	};
}