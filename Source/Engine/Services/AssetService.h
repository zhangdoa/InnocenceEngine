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

		static bool Import(const char* fileName, const char* exportPath);

		static bool Load(const char* fileName, TransformComponent& component);
		static bool Load(const char* fileName, ModelComponent& component);
		static bool Load(const char* fileName, DrawCallComponent& component);
		static bool Load(const char* fileName, MeshComponent& component);
		static bool Load(const char* fileName, MaterialComponent& component);
		static bool Load(const char* fileName, TextureComponent& component);
		// static bool Load(const char* fileName, SkeletonComponent& component);
		// static bool Load(const char* fileName, AnimationComponent& component);
		static bool Load(const char* fileName, CameraComponent& component);
		static bool Load(const char* fileName, LightComponent& component);

		static bool SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData);
	};
}