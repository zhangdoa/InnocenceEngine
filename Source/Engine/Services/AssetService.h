#pragma once
#include "../Interface/IService.h"
#include "../Common/ComponentHeaders.h"
#include "../Common/AssetData.h"
#include "../Common/AssetImportData.h"
#include "../Common/BCCompression.h"

namespace Inno
{
	class AssetService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AssetService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		static MeshAssetHandle AllocateMeshAsset(const char* name, ObjectLifespan lifespan);
		static MeshAssetData* GetMeshAsset(MeshAssetHandle handle);
		static MeshAssetHandle FindMeshAsset(const char* name);
		static uint32_t DebugGetMeshGeneration(uint32_t index);

		// TASK-27: AllocateMaterialAsset returns `{handle, wasNewlyCreated}`. Callers that
		// need a clean slate (load paths populating the asset from a source-of-truth)
		// must branch on `wasNewlyCreated == false` and reset the existing data
		// themselves — the allocator itself is `get-or-create`, not `replace`, because
		// material assets are shared by name across components.
		struct MaterialAssetAllocation
		{
			MaterialAssetHandle m_Handle;
			bool m_WasNewlyCreated;
		};
		static MaterialAssetAllocation AllocateMaterialAsset(const char* name, ObjectLifespan lifespan);
		static MaterialAssetData* GetMaterialAsset(MaterialAssetHandle handle);
		static MaterialAssetHandle FindMaterialAsset(const char* name);

		static TextureAssetHandle AllocateTextureAsset(const char* name, ObjectLifespan lifespan);
		static TextureAssetData* GetTextureAsset(TextureAssetHandle handle);
		static TextureAssetHandle FindTextureAsset(const char* name);

		static void ReleaseAssetsByLifespan(ObjectLifespan lifespan);

		static std::string GetAssetFilePath(const char* componentName);
		static std::string GetBinaryFilePath(const char* binaryFileName);
		static std::string GetComponentDirectory();

		static bool Import(const char* fileName);
		// Synchronous variant: submits the import task and blocks until it completes.
		// Intended for offline tools and tests. Returns false for unsupported formats.
		static bool ImportSync(const char* fileName);

		static bool SaveScene(const char* fileName);
		static bool LoadScene(const char* fileName);

		static bool Load(const char* fileName, TransformComponent& component);
		static bool Load(const char* fileName, MeshComponent& component, EntityID owner);
		static bool Load(const char* fileName, MaterialComponent& component, EntityID owner);
		static bool Load(const char* fileName, TextureComponent& component, EntityID owner);
		// static bool Load(const char* fileName, SkeletonComponent& component);
		// static bool Load(const char* fileName, AnimationComponent& component);
		static bool Load(const char* fileName, CameraComponent& component);
		static bool Load(const char* fileName, LightComponent& component);

		static bool Save(const MeshComponent& component, std::vector<Vertex>& vertices, std::vector<Index>& indices);
		static bool Save(const MaterialComponent& component);
		static bool Save(const TextureComponent& component, void* textureData);
		static bool Save(const CameraComponent& component);
		static bool Save(const LightComponent& component);

		static bool Save(const char* fileName, const TextureDesc& textureDesc, void* textureData);

		// Load a standalone PNG/etc, BC-compress it for the given material
		// slot, write {Generated/Components/<instanceName>.json + .innobin}.
		// Returns the saved instance name on success, empty on failure.
		// bc4Source picks which RGBA channel is packed into BC4 (slot 2/3/4);
		// default R matches separate single-channel PNGs that STB broadcasts
		// to RGBA on load. glTF MetallicRoughness packing needs B for metallic
		// (slot 2) and G for roughness (slot 3).
		static std::string ImportTexture(const char*          absolutePath,
		                                 TextureSampler       sampler,
		                                 TextureUsage         usage,
		                                 bool                 isSRGB,
		                                 uint32_t             slotIndex,
		                                 const char*          instanceName,
		                                 TextureChannelSource bc4Source = TextureChannelSource::R);
	};
}
