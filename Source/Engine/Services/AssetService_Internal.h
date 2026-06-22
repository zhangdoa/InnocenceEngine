#pragma once
#include "../Common/Array.h"
#include "../Common/Deque.h"
#include "../Common/UnorderedSet.h"
#include "AssetService.h"
#include "../Common/HashMap.h"
#include <atomic>

namespace Inno
{
	namespace AssetServiceNS
	{
		extern ObjectStatus m_ObjectStatus;

		// Inno::Deque preserves pointer stability across emplace_back so GetMeshAsset
		// returns stable pointers while Allocate concurrently appends.
		extern Inno::Deque<MeshAsset> m_MeshAssets;
		extern Inno::Array<uint32_t> m_MeshFreeSlots;
		extern Inno::Array<uint32_t> m_MeshGenerations;
		extern Inno::HashMap<std::string, MeshAssetHandle> m_MeshLUT;
		extern std::shared_mutex s_MeshMutex;

		// Monotonic version of the resident mesh set: bumped on each mesh
		// Resident/Released transition. The MeshGeometry table is static per-mesh
		// data, so DrawCallService stages + uploads it only when this changes
		// rather than rebuilding it every frame.
		extern std::atomic<uint64_t> s_MeshResidencyEpoch;

		extern Inno::Deque<MaterialAsset> m_MaterialAssets;
		extern Inno::Array<uint32_t> m_MaterialFreeSlots;
		extern Inno::Array<uint32_t> m_MaterialGenerations;
		extern Inno::HashMap<std::string, MaterialAssetHandle> m_MaterialLUT;
		extern std::shared_mutex s_MaterialMutex;

		extern Inno::Deque<TextureAsset> m_TextureAssets;
		extern Inno::Array<uint32_t> m_TextureFreeSlots;
		extern Inno::Array<uint32_t> m_TextureGenerations;
		extern Inno::HashMap<std::string, TextureAssetHandle> m_TextureLUT;
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
		extern Inno::UnorderedSet<std::string> s_ImportTextureDedup;
	}
}
