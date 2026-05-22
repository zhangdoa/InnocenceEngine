#pragma once
#include "Array.h"
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

	struct MeshAsset
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

	struct TextureAsset
	{
		ObjectName m_Name;
		ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
		AssetResidency m_Residency = AssetResidency::Unloaded;

		TextureDesc m_TextureDesc;
	};

	struct MaterialAsset
	{
		ObjectName m_Name;
		ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
		AssetResidency m_Residency = AssetResidency::Unloaded;

		MaterialAttributes m_Attributes;
		Inno::Array<std::string> m_TextureNames;
		Inno::Array<TextureAssetHandle> m_TextureSlots;
		ShaderModel m_ShaderModel = ShaderModel::Invalid;
	};
}
