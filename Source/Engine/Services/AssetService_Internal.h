#pragma once
#include "AssetService.h"

namespace Inno
{
	namespace AssetServiceNS
	{
		extern ObjectStatus m_ObjectStatus;

		// Mesh asset registry
		// deque guarantees reference/pointer stability on push_back, unlike vector.
		// This allows GetMeshAsset to return stable pointers while Allocate appends concurrently.
		extern std::deque<MeshAssetData> m_MeshAssets;
		extern std::vector<uint32_t> m_MeshFreeSlots;
		extern std::vector<uint32_t> m_MeshGenerations;
		extern std::unordered_map<std::string, MeshAssetHandle> m_MeshLUT;
		extern std::shared_mutex s_MeshMutex;

		// Material asset registry
		extern std::deque<MaterialAssetData> m_MaterialAssets;
		extern std::vector<uint32_t> m_MaterialFreeSlots;
		extern std::vector<uint32_t> m_MaterialGenerations;
		extern std::unordered_map<std::string, MaterialAssetHandle> m_MaterialLUT;
		extern std::shared_mutex s_MaterialMutex;

		// Texture asset registry
		extern std::deque<TextureAssetData> m_TextureAssets;
		extern std::vector<uint32_t> m_TextureFreeSlots;
		extern std::vector<uint32_t> m_TextureGenerations;
		extern std::unordered_map<std::string, TextureAssetHandle> m_TextureLUT;
		extern std::shared_mutex s_TextureMutex;

		// Dedup guard for concurrent ImportTexture calls targeting the same instance
		// name. When the scene-level fan-out submits a task per (mesh, material,
		// texture), two materials referencing the same source texture produce the
		// same deterministic instanceName. Without this, both tasks race on
		// decode+compress+Save. First caller wins; subsequent callers short-circuit
		// and return the name assuming the first succeeded. If the first fails, the
		// error log is the user-visible signal — re-running -bake starts from a
		// fresh process and clears the set.
		extern std::mutex s_ImportTextureDedupMutex;
		extern std::unordered_set<std::string> s_ImportTextureDedup;
	}
}
