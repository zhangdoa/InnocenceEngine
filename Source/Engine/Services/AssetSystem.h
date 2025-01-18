#pragma once
#include "../Interface/ISystem.h"
#include "../Component/MeshComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/SkeletonComponent.h"
#include "../Component/AnimationComponent.h"
#include "../Component/ModelComponent.h"

namespace Inno
{
	class AssetSystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AssetSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool OnFrameEnd() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool ConvertModel(const char* fileName, const char* exportPath);
		Model* LoadModel(const char* fileName, bool AsyncUploadGPUResource = true);
		TextureComponent* LoadTexture(const char* fileName);
		bool SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData);
		bool SaveTexture(const char* fileName, TextureComponent* TextureComp);

		bool LoadAssetsForComponents(bool AsyncLoad);

		bool RecordLoadedRenderableSet(const char* fileName, RenderableSet* pair);
		bool FindLoadedRenderableSet(const char* fileName, RenderableSet*& pair);

		bool RecordLoadedModel(const char* fileName, Model* model);
		bool FindLoadedModel(const char* fileName, Model*& model);

		bool RecordLoadedTexture(const char* fileName, TextureComponent* texture);
		bool FindLoadedTexture(const char* fileName, TextureComponent*& texture);

		bool RecordLoadedSkeleton(const char* fileName, SkeletonComponent* skeleton);
		bool FindLoadedSkeleton(const char* fileName, SkeletonComponent*& skeleton);

		bool RecordLoadedAnimation(const char* fileName, AnimationComponent* animation);
		bool FindLoadedAnimation(const char* fileName, AnimationComponent*& animation);

		ArrayRangeInfo AddRenderableSets(uint64_t count);
		RenderableSet* GetRenderableSet(uint64_t index);

		Model* AddModel();

		Model* AddModel(MeshShape shape, ShaderModel shaderModel);
	};
}