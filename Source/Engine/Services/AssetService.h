#pragma once
#include "../Interface/ISystem.h"
#include "../Common/ComponentHeaders.h"

namespace Inno
{
	class AssetService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AssetService);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool Import(const char* fileName, const char* exportPath);


		bool Load(const char* fileName, TransformComponent& component);
		bool Load(const char* fileName, ModelComponent& component);
		bool Load(const char* fileName, MeshComponent& component);
		bool Load(const char* fileName, MaterialComponent& component);
		bool Load(const char* fileName, TextureComponent& component);
		// bool Load(const char* fileName, SkeletonComponent& component);
		// bool Load(const char* fileName, AnimationComponent& component);
		bool Load(const char* fileName, CameraComponent& component);
		bool Load(const char* fileName, LightComponent& component);

		bool SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData);
	};
}