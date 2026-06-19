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
}

bool DrawCallServiceImpl::UpdateDrawCalls()
{
	StageMeshGeometries();
	CollectVisibleInstances();
	return true;
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
	uint32_t l_drawCallIndex = 0;
	for (size_t i = 0; i < l_Meshes.size(); i++)
	{
		EntityID l_Entity = l_Owners[i];
		const MeshComponent& l_mesh = l_Meshes[i];

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

		// Per-visible-instance record. Geometry (vertex/index buffers,
		// counts, strides) lives in the MeshGeometry table, looked up by
		// m_meshID; the cull pass writes the indirect draw command, so this
		// record carries only material, meshID, and the world-space AABB.
		auto* l_world = l_registry->Get<WorldTransformComponent>(l_Entity);
		const AABB& l_localAabb = l_vis ? l_vis->m_AABB : l_resource->m_AABB;
		auto* l_materialAsset = AssetService::GetMaterialAsset(l_material->m_Asset);

		RenderInstance l_renderInstance;
		l_renderInstance.PoisonInit();

		l_renderInstance.m_MaterialIndex = l_drawCallIndex;
		l_renderInstance.m_meshID = l_meshID;

		if (l_world)
		{
			const auto& M = l_world->m_WorldMatrix;
			Vec4 l_corners[8] = {
				Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMin.z, 1.0f),
				Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMin.z, 1.0f),
				Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMin.z, 1.0f),
				Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMin.z, 1.0f),
				Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMax.z, 1.0f),
				Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMax.z, 1.0f),
				Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMax.z, 1.0f),
				Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMax.z, 1.0f),
			};
			Vec4 wsMin = Math::maxVec4<float>;
			Vec4 wsMax = Math::minVec4<float>;
			for (int i = 0; i < 8; i++)
			{
				Vec4 ws = M * l_corners[i];
				wsMin = Math::elementWiseMin(wsMin, ws);
				wsMax = Math::elementWiseMax(wsMax, ws);
			}
			wsMin.w = 1.0f;
			wsMax.w = 1.0f;
			l_renderInstance.m_BoundingBoxMin = wsMin;
			l_renderInstance.m_BoundingBoxMax = wsMax;
		}
		else
		{
			l_renderInstance.m_BoundingBoxMin = Vec4(l_localAabb.m_boundMin.x, l_localAabb.m_boundMin.y, l_localAabb.m_boundMin.z, 1.0f);
			l_renderInstance.m_BoundingBoxMax = Vec4(l_localAabb.m_boundMax.x, l_localAabb.m_boundMax.y, l_localAabb.m_boundMax.z, 1.0f);
		}

		m_RenderInstanceVector.emplace_back(l_renderInstance);

		TransformConstantBuffer l_transformCB;
		l_transformCB.PoisonInit();
		if (l_world)
		{
			l_transformCB.m = l_world->m_WorldMatrix;
			l_transformCB.normalMat = l_world->m_WorldRotationMatrix;
		}
		else
		{
			l_transformCB.m = Mat4();
			l_transformCB.normalMat = Mat4();
		}
		m_TransformBufferVector.emplace_back(l_transformCB);

		MaterialConstantBuffer l_materialCB;
		l_materialCB.PoisonInit();
		l_materialCB.m_MaterialType = 0;
		if (l_materialAsset)
			l_materialCB.m_MaterialAttributes = l_materialAsset->m_Attributes;

		for (size_t j = 0; j < MaxTextureSlotCount; j++)
		{
			l_materialCB.m_TextureIndices[j] = INVALID_TEXTURE_INDEX;
		}

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

				auto textureIndex = l_textureService->GetIndex(l_texture, Accessibility::ReadOnly);
				l_materialCB.m_TextureIndices[j] = textureIndex.value_or(INVALID_TEXTURE_INDEX);
			}
		}

		m_MaterialCBVector.emplace_back(l_materialCB);

		l_drawCallIndex++;
	}

}
