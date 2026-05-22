#pragma once
#include <cstdint>
#include <functional>

namespace Inno
{
	struct MeshAsset;
	struct TextureAsset;
	struct MaterialAsset;

	template<typename T>
	struct AssetHandle
	{
		uint32_t m_Index = UINT32_MAX;
		uint32_t m_Generation = 0;

		bool IsValid() const { return m_Index != UINT32_MAX; }

		bool operator==(const AssetHandle& other) const
		{
			return m_Index == other.m_Index && m_Generation == other.m_Generation;
		}
		bool operator!=(const AssetHandle& other) const { return !(*this == other); }
	};

	using MeshAssetHandle = AssetHandle<MeshAsset>;
	using TextureAssetHandle = AssetHandle<TextureAsset>;
	using MaterialAssetHandle = AssetHandle<MaterialAsset>;
}

namespace std
{
	template<typename T>
	struct hash<Inno::AssetHandle<T>>
	{
		size_t operator()(const Inno::AssetHandle<T>& h) const
		{
			return hash<uint64_t>()(static_cast<uint64_t>(h.m_Index) << 32 | h.m_Generation);
		}
	};
}
