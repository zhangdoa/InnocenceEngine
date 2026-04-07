#pragma once
#include "../Interface/IService.h"
#include "../Common/ComponentHeaders.h"
#include "../Common/AssetData.h"

namespace Inno
{
	class AssetService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AssetService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		// Asset registry — allocate, query, release by lifespan
		static MeshAssetHandle AllocateMeshAsset(const char* name, ObjectLifespan lifespan);
		static MeshAssetData* GetMeshAsset(MeshAssetHandle handle);
		static MeshAssetHandle FindMeshAsset(const char* name);

		static MaterialAssetHandle AllocateMaterialAsset(const char* name, ObjectLifespan lifespan);
		static MaterialAssetData* GetMaterialAsset(MaterialAssetHandle handle);
		static MaterialAssetHandle FindMaterialAsset(const char* name);

		static TextureAssetHandle AllocateTextureAsset(const char* name, ObjectLifespan lifespan);
		static TextureAssetData* GetTextureAsset(TextureAssetHandle handle);
		static TextureAssetHandle FindTextureAsset(const char* name);

		static void ReleaseAssetsByLifespan(ObjectLifespan lifespan);

		// Serialization
		static bool Import(const char* fileName);

		static bool SaveScene(const char* fileName);
		static bool LoadScene(const char* fileName);

		static std::string GetAssetFilePath(const char* componentName);
		static std::string GetBinaryFilePath(const char* binaryFileName);
		static std::string GetComponentDirectory();

		static bool Load(const char* fileName, TransformComponent& component);
		static bool Load(const char* fileName, MeshComponent& component, EntityID owner = INVALID_ENTITY);
		static bool Load(const char* fileName, MaterialComponent& component, EntityID owner = INVALID_ENTITY);
		static bool Load(const char* fileName, TextureComponent& component, EntityID owner = INVALID_ENTITY);
		static bool Load(const char* fileName, CameraComponent& component);
		static bool Load(const char* fileName, LightComponent& component);

		static bool Save(const MeshComponent& component, std::vector<Vertex>& vertices, std::vector<Index>& indices);
		static bool Save(const MaterialComponent& component);
		static bool Save(const TextureComponent& component, void* textureData);
		static bool Save(const CameraComponent& component);
		static bool Save(const LightComponent& component);

		static bool Save(const char* fileName, const TextureDesc& textureDesc, void* textureData);
	};
}