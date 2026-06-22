#include "DrawCallService.h"
#include "DrawCallServiceImpl.h"

#include "../Common/LogService.h"
#include "../Common/Array.h"
#include "EntityRegistry.h"
#include "RenderingConfigurationService.h"
#include "AssetService.h"
#include "AssetService_Internal.h"
#include "../Component/MeshComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/WorldTransformComponent.h"
#include "../Component/VisibilityComponent.h"
#include "../Engine.h"
#include "GPUBufferResourceService.h"
#include "TextureResourceService.h"
#include "FrameManagementService.h"

using namespace Inno;

// Stage the per-mesh geometry table from resident assets. Non-Resident assets are
// skipped: a released Deque slot may be reused, so a stale entry would no longer
// point at the same mesh.
namespace
{
	void StageMeshGeometryTable(Inno::Array<MeshGeometry>& outTable)
	{
		std::shared_lock<std::shared_mutex> l_lock(AssetServiceNS::s_MeshMutex);
		const auto& l_assets = AssetServiceNS::m_MeshAssets;
		for (size_t i = 0; i < l_assets.size(); i++)
		{
			const auto& l_asset = l_assets[i];
			if (l_asset.m_Residency != AssetResidency::Resident)
				continue;

			MeshGeometry l_mesh;
			l_mesh.PoisonInit();
			l_mesh.m_VertexBufferAddress = l_asset.m_VertexBufferView.m_BufferLocation;
			l_mesh.m_IndexBufferAddress = l_asset.m_IndexBufferView.m_BufferLocation;
			if (l_asset.m_VertexBufferView.m_StrideInBytes == 0)
			{
				Log(Error, "Vertex stride is zero - cannot calculate vertex count for asset '", l_asset.m_Name.c_str(), "'");
				l_mesh.m_VertexCount = 0;
			}
			else
			{
				l_mesh.m_VertexCount = l_asset.m_VertexBufferView.m_SizeInBytes / l_asset.m_VertexBufferView.m_StrideInBytes;
			}
			l_mesh.m_IndexCount = l_asset.GetIndexCount();
			l_mesh.m_VertexStride = l_asset.m_VertexBufferView.m_StrideInBytes;
			l_mesh.m_IndexStride = l_asset.m_IndexBufferView.m_StrideInBytes;
			outTable.emplace_back(l_mesh);
		}
	}

	// Returns UINT32_MAX if the asset has no table entry (caller skips the entity).
	uint32_t FindMeshIDForAsset(const Inno::Array<MeshGeometry>& table, const MeshAsset& asset)
	{
		const auto l_vbAddr = asset.m_VertexBufferView.m_BufferLocation;
		const auto l_vtxCount = asset.GetVertexCount();
		for (uint32_t mi = 0; mi < table.size(); mi++)
		{
			if (table[mi].m_VertexBufferAddress == l_vbAddr
				&& table[mi].m_VertexCount == l_vtxCount)
			{
				return mi;
			}
		}
		return UINT32_MAX;
	}

	// A fully-resolved render candidate: every field is decided by the imperative
	// shell (cull-or-substitute already applied), so the producers below are pure
	// a -> b transforms with no conditionals — the functional core.
	struct ResolvedRenderRecord
	{
		uint32_t m_InstanceIndex;
		uint32_t m_MeshID;
		Mat4 m_WorldMatrix;
		Mat4 m_NormalMatrix;
		AABB m_LocalAABB;
		MaterialAttributes m_MaterialAttributes;
		uint32_t m_TextureIndices[MaxTextureSlotCount];
	};

	RenderInstance ToRenderInstance(const ResolvedRenderRecord& record)
	{
		RenderInstance l_out;
		l_out.PoisonInit();
		l_out.m_MaterialIndex = record.m_InstanceIndex;
		l_out.m_meshID = record.m_MeshID;

		const AABB& l_aabb = record.m_LocalAABB;
		const Vec4 l_corners[8] = {
			Vec4(l_aabb.m_boundMin.x, l_aabb.m_boundMin.y, l_aabb.m_boundMin.z, 1.0f),
			Vec4(l_aabb.m_boundMax.x, l_aabb.m_boundMin.y, l_aabb.m_boundMin.z, 1.0f),
			Vec4(l_aabb.m_boundMin.x, l_aabb.m_boundMax.y, l_aabb.m_boundMin.z, 1.0f),
			Vec4(l_aabb.m_boundMax.x, l_aabb.m_boundMax.y, l_aabb.m_boundMin.z, 1.0f),
			Vec4(l_aabb.m_boundMin.x, l_aabb.m_boundMin.y, l_aabb.m_boundMax.z, 1.0f),
			Vec4(l_aabb.m_boundMax.x, l_aabb.m_boundMin.y, l_aabb.m_boundMax.z, 1.0f),
			Vec4(l_aabb.m_boundMin.x, l_aabb.m_boundMax.y, l_aabb.m_boundMax.z, 1.0f),
			Vec4(l_aabb.m_boundMax.x, l_aabb.m_boundMax.y, l_aabb.m_boundMax.z, 1.0f),
		};
		Vec4 l_wsMin = Math::maxVec4<float>;
		Vec4 l_wsMax = Math::minVec4<float>;
		for (int i = 0; i < 8; i++)
		{
			Vec4 l_ws = record.m_WorldMatrix * l_corners[i];
			l_wsMin = Math::elementWiseMin(l_wsMin, l_ws);
			l_wsMax = Math::elementWiseMax(l_wsMax, l_ws);
		}
		l_wsMin.w = 1.0f;
		l_wsMax.w = 1.0f;
		l_out.m_BoundingBoxMin = l_wsMin;
		l_out.m_BoundingBoxMax = l_wsMax;
		return l_out;
	}

	TransformConstantBuffer ToTransformConstantBuffer(const ResolvedRenderRecord& record)
	{
		TransformConstantBuffer l_out;
		l_out.PoisonInit();
		l_out.m = record.m_WorldMatrix;
		l_out.normalMat = record.m_NormalMatrix;
		return l_out;
	}

	MaterialConstantBuffer ToMaterialConstantBuffer(const ResolvedRenderRecord& record)
	{
		MaterialConstantBuffer l_out;
		l_out.PoisonInit();
		l_out.m_MaterialType = 0;
		l_out.m_MaterialAttributes = record.m_MaterialAttributes;
		for (size_t j = 0; j < MaxTextureSlotCount; j++)
			l_out.m_TextureIndices[j] = record.m_TextureIndices[j];
		return l_out;
	}
}

void DrawCallServiceImpl::StageMeshGeometries()
{
	m_MeshGeometryVector.clear();
	StageMeshGeometryTable(m_MeshGeometryVector);
}

void DrawCallServiceImpl::CollectVisibleInstances()
{
	m_RenderInstanceVector.clear();
	m_TransformBufferVector.clear();
	m_MaterialCBVector.clear();

	auto l_textureService = g_Engine->Get<TextureResourceService>();
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_MeshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_Meshes = l_MeshStorage.All();
	const auto& l_Owners = l_MeshStorage.AllOwners();
	uint32_t l_instanceIndex = 0;
	for (size_t i = 0; i < l_Meshes.size(); i++)
	{
		EntityID l_Entity = l_Owners[i];
		const MeshComponent& l_mesh = l_Meshes[i];

		// Imperative shell: resolve the domain (mesh asset, material, transform,
		// meshID) and decide cull-or-substitute. Below the resolve block the record
		// is fully decided and fed to the pure producers — no conditionals there.
		if (l_mesh.m_ObjectStatus != ObjectStatus::Activated)
			continue;

		auto* l_resource = AssetService::GetMeshAsset(l_mesh.m_Asset);
		if (!l_resource || l_resource->m_Residency != AssetResidency::Resident)
			continue;

		auto* l_material = l_registry->Get<MaterialComponent>(l_Entity);
		if (!l_material)
			continue;

		auto* l_vis = l_registry->Get<VisibilityComponent>(l_Entity);
		if (l_vis && !l_vis->m_Visible)
			continue;

		const uint32_t l_meshID = FindMeshIDForAsset(m_MeshGeometryVector, *l_resource);
		if (l_meshID == UINT32_MAX)
			continue;

		auto* l_world = l_registry->Get<WorldTransformComponent>(l_Entity);
		auto* l_materialAsset = AssetService::GetMaterialAsset(l_material->m_Asset);

		ResolvedRenderRecord l_record;
		l_record.m_InstanceIndex = l_instanceIndex;
		l_record.m_MeshID = l_meshID;
		// Substitute identity for a missing transform: it keeps the producer
		// conditional-free and renders the instance at the origin rather than the
		// degenerate zero matrix the old no-transform branch left behind.
		l_record.m_WorldMatrix = l_world ? l_world->m_WorldMatrix : Math::generateIdentityMatrix<float>();
		l_record.m_NormalMatrix = l_world ? l_world->m_WorldRotationMatrix : Math::generateIdentityMatrix<float>();
		l_record.m_LocalAABB = l_vis ? l_vis->m_AABB : l_resource->m_AABB;
		// Default to zeroed attributes when the material asset is missing so the
		// validator's dword scan stays silent (leaving them poisoned was a latent bug).
		l_record.m_MaterialAttributes = l_materialAsset ? l_materialAsset->m_Attributes : MaterialAttributes{};

		for (size_t j = 0; j < MaxTextureSlotCount; j++)
			l_record.m_TextureIndices[j] = INVALID_TEXTURE_INDEX;

		if (l_materialAsset)
		{
			// The CPU asset layer allows arbitrarily many texture names, but
			// MaterialConstantBuffer::m_TextureIndices is fixed at MaxTextureSlotCount.
			// Log once per frame when a material is over limit so silent truncation is visible.
			if (l_materialAsset->m_TextureNames.size() > MaxTextureSlotCount)
			{
				Log(Warning, "Material '", l_materialAsset->m_Name.c_str(),
					"' has ", l_materialAsset->m_TextureNames.size(),
					" textures but GPU layout supports only ", MaxTextureSlotCount,
					"; extra entries are ignored.");
			}
			for (size_t j = 0; j < l_materialAsset->m_TextureNames.size() && j < MaxTextureSlotCount; j++)
			{
				const auto& l_textureName = l_materialAsset->m_TextureNames[j];
				if (l_textureName.empty())
					continue;

				auto l_texture = l_textureService->Find(l_textureName.c_str());
				if (!l_texture || l_texture->m_ObjectStatus != ObjectStatus::Activated)
					continue;

				auto l_textureIndex = l_textureService->GetIndex(l_texture, Accessibility::ReadOnly);
				l_record.m_TextureIndices[j] = l_textureIndex.value_or(INVALID_TEXTURE_INDEX);
			}
		}

		// Functional core: pure a -> b from the fully-resolved record, no conditionals.
		m_RenderInstanceVector.emplace_back(ToRenderInstance(l_record));
		m_TransformBufferVector.emplace_back(ToTransformConstantBuffer(l_record));
		m_MaterialCBVector.emplace_back(ToMaterialConstantBuffer(l_record));

		l_instanceIndex++;
	}
}
