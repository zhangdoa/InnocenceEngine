#pragma once
#include "GPUDataStructure.h"
#include "AssetHandle.h"
#include "Object.h"

namespace Inno
{
	enum class AssetResidency : uint8_t
	{
		Unloaded,
		Loading,
		Resident,
		Released
	};

	struct MeshAssetData
	{
		ObjectName m_Name;
		ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
		AssetResidency m_Residency = AssetResidency::Unloaded;

		GPUBufferView m_VertexBufferView;
		GPUBufferView m_IndexBufferView;

		void* m_MappedMemory_VB = nullptr;
		void* m_MappedMemory_IB = nullptr;

		AABB m_AABB;

		uint32_t GetVertexCount() const
		{
			if (m_VertexBufferView.m_StrideInBytes == 0)
				return 0;
			return m_VertexBufferView.m_SizeInBytes / m_VertexBufferView.m_StrideInBytes;
		}

		uint32_t GetIndexCount() const
		{
			if (m_IndexBufferView.m_StrideInBytes == 0)
				return 0;
			return m_IndexBufferView.m_SizeInBytes / m_IndexBufferView.m_StrideInBytes;
		}
	};

	struct TextureAssetData
	{
		ObjectName m_Name;
		ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
		AssetResidency m_Residency = AssetResidency::Unloaded;

		TextureDesc m_TextureDesc;
	};

	struct MaterialAssetData
	{
		ObjectName m_Name;
		ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
		AssetResidency m_Residency = AssetResidency::Unloaded;

		MaterialAttributes m_Attributes;
		std::vector<TextureAssetHandle> m_TextureSlots;
		ShaderModel m_ShaderModel = ShaderModel::Invalid;
	};
}
