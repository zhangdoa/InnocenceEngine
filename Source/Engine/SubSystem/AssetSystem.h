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

		bool ConvertModel(const char* fileName, const char* exportPath) override;
		Model* LoadModel(const char* fileName, bool AsyncUploadGPUResource = true) override;
		TextureComponent* LoadTexture(const char* fileName) override;
		bool SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData) override;
		bool SaveTexture(const char* fileName, TextureComponent* TextureComp) override;

		bool LoadAssetsForComponents(bool AsyncLoad) override;

		bool RecordLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair* pair) override;
		bool FindLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair*& pair) override;

		bool RecordLoadedModel(const char* fileName, Model* model) override;
		bool FindLoadedModel(const char* fileName, Model*& model) override;

		bool RecordLoadedTexture(const char* fileName, TextureComponent* texture) override;
		bool FindLoadedTexture(const char* fileName, TextureComponent*& texture) override;

		bool RecordLoadedSkeleton(const char* fileName, SkeletonComponent* skeleton) override;
		bool FindLoadedSkeleton(const char* fileName, SkeletonComponent*& skeleton) override;

		bool RecordLoadedAnimation(const char* fileName, AnimationComponent* animation) override;
		bool FindLoadedAnimation(const char* fileName, AnimationComponent*& animation) override;

		ArrayRangeInfo AddMeshMaterialPairs(uint64_t count) override;
		MeshMaterialPair* GetMeshMaterialPair(uint64_t index) override;

		Model* AddModel() override;

		Model* AddProceduralModel(ProceduralMeshShape shape, ShaderModel shaderModel) override;
		bool GenerateProceduralMesh(ProceduralMeshShape shape, MeshComponent* meshComponent) override;
		void FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace) override;
	};
}