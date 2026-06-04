#include "PTPass.h"
#include "../../Engine/Common/Array.h"

#include <vector>

#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/MeshResourceService.h"
#include "../../Engine/Services/DX12/DX12MeshResourceService.h"
#include "../../Engine/Component/MeshComponent.h"
#include "../../Engine/Component/MaterialComponent.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

void PTPass::RebuildMaterialBuffer()
{
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_meshOwners = l_meshStorage.AllOwners();

	if (l_meshOwners.empty())
		return;

	m_PendingMaterials.clear();
	auto& l_materials = m_PendingMaterials;

	// Iteration order + skip predicate must match UpdateRaytracingInstances
	// (DX12GPUBufferResourceService) exactly — l_materials[i] is the material
	// for TLAS instance i, and ClosestHit reads in_MaterialBuffer[InstanceIndex()].
	auto* l_meshService = static_cast<DX12MeshResourceService*>(g_Engine->Get<MeshResourceService>());
	for (EntityID l_entity : l_meshOwners)
	{
		auto* l_mesh = l_registry->Get<MeshComponent>(l_entity);
		if (!l_mesh || !l_mesh->m_Asset.IsValid())
			continue;
		if (l_mesh->m_ObjectStatus != ObjectStatus::Activated)
			continue;
		if (l_meshService->GetBLASAddress(l_mesh->m_Asset) == 0)
			continue;
		if (l_meshService->GetVertexSRVSlot(l_mesh->m_Asset) == UINT32_MAX)
			continue;

		MaterialConstantBuffer l_materialCB = {};
		auto* l_matComp = l_registry->Get<MaterialComponent>(l_entity);
		if (l_matComp == nullptr)
		{
			if (m_WarnedMissingMaterial.insert(l_entity).second)
				Log(Warning, "PT: entity '", l_registry->GetName(l_entity),
					"' (TLAS instance ", l_materials.size(), ") has a MeshComponent but no "
					"MaterialComponent — falling back to default white Lambert.");
		}
		else
		{
			auto* l_matAsset = AssetService::GetMaterialAsset(l_matComp->m_Asset);
			if (l_matAsset == nullptr)
			{
				if (m_WarnedMissingMaterial.insert(l_entity).second)
					Log(Warning, "PT: entity '", l_registry->GetName(l_entity),
						"' (TLAS instance ", l_materials.size(), ") MaterialComponent has "
						"unresolvable asset handle — falling back to default white Lambert.");
			}
			else
			{
				l_materialCB.m_MaterialAttributes = l_matAsset->m_Attributes;
			}
		}
		for (size_t j = 0; j < MaxTextureSlotCount; j++)
			l_materialCB.m_TextureIndices[j] = INVALID_TEXTURE_INDEX;

		if (l_matComp)
		{
			auto* l_matAsset = AssetService::GetMaterialAsset(l_matComp->m_Asset);
			if (l_matAsset)
			{
				auto* l_texService = g_Engine->Get<TextureResourceService>();
				for (size_t j = 0; j < l_matAsset->m_TextureNames.size() && j < MaxTextureSlotCount; j++)
				{
					const auto& l_textureName = l_matAsset->m_TextureNames[j];
					if (l_textureName.empty())
						continue;
					auto l_texture = l_texService->Find(l_textureName.c_str());
					if (!l_texture || l_texture->m_ObjectStatus != ObjectStatus::Activated)
						continue;
					auto l_idx = l_texService->GetIndex(l_texture, Accessibility::ReadOnly);
					l_materialCB.m_TextureIndices[j] = l_idx.value_or(INVALID_TEXTURE_INDEX);
				}
			}
		}

		l_materials.push_back(l_materialCB);
	}

	if (l_materials.empty())
		return;

	auto l_bufService = g_Engine->Get<GPUBufferResourceService>();

	if (m_MaterialBuffer)
	{
		l_bufService->Delete(m_MaterialBuffer);
		m_MaterialBuffer = nullptr;
	}

	m_MaterialBuffer = l_bufService->Add("PTMaterialBuffer");
	m_MaterialBuffer->m_ElementCount     = l_materials.size();
	m_MaterialBuffer->m_ElementSize      = sizeof(MaterialConstantBuffer);
	m_MaterialBuffer->m_CPUAccessibility = Accessibility::WriteOnly;
	m_MaterialBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
	m_MaterialBuffer->m_InitialData      = l_materials.data();
	l_bufService->Initialize(m_MaterialBuffer);

	m_BuiltMeshCount = l_meshOwners.size();
}

void PTPass::RefreshMaterialTextureIndices()
{
	// Re-resolve every material's bindless texture indices and re-upload
	// the material buffer. Iterates the same mesh owners in the same order
	// as RebuildGeometryBuffers (m_BuiltMeshCount gate — skip until first
	// rebuild completed, so TLAS instance ordering matches). Textures that
	// were pending when the rebuild ran get their correct bindless index
	// here on later frames once InitializeComponents Activates them.
	if (m_BuiltMeshCount == 0) return;
	if (!m_MaterialBuffer || m_MaterialBuffer->m_ObjectStatus != ObjectStatus::Activated) return;

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_meshOwners = l_meshStorage.AllOwners();
	auto* l_texService = g_Engine->Get<TextureResourceService>();

	Inno::Array<MaterialConstantBuffer> l_materials;
	l_materials.reserve(m_BuiltMeshCount);

	for (EntityID l_entity : l_meshOwners)
	{
		auto* l_mesh = l_registry->Get<MeshComponent>(l_entity);
		if (!l_mesh || !l_mesh->m_Asset.IsValid()) continue;
		if (l_mesh->m_ObjectStatus != ObjectStatus::Activated) continue;

		const auto* l_resource = AssetService::GetMeshAsset(l_mesh->m_Asset);
		if (!l_resource || l_resource->m_Residency != AssetResidency::Resident) continue;

		MaterialConstantBuffer l_materialCB = {};
		auto* l_matComp = l_registry->Get<MaterialComponent>(l_entity);
		if (l_matComp)
		{
			auto* l_matAsset = AssetService::GetMaterialAsset(l_matComp->m_Asset);
			if (l_matAsset)
				l_materialCB.m_MaterialAttributes = l_matAsset->m_Attributes;
		}
		for (size_t j = 0; j < MaxTextureSlotCount; j++)
			l_materialCB.m_TextureIndices[j] = INVALID_TEXTURE_INDEX;

		if (l_matComp)
		{
			auto* l_matAsset = AssetService::GetMaterialAsset(l_matComp->m_Asset);
			if (l_matAsset)
			{
				for (size_t j = 0; j < l_matAsset->m_TextureNames.size() && j < MaxTextureSlotCount; j++)
				{
					const auto& l_textureName = l_matAsset->m_TextureNames[j];
					if (l_textureName.empty()) continue;
					auto l_texture = l_texService->Find(l_textureName.c_str());
					if (!l_texture || l_texture->m_ObjectStatus != ObjectStatus::Activated) continue;
					auto l_idx = l_texService->GetIndex(l_texture, Accessibility::ReadOnly);
					l_materialCB.m_TextureIndices[j] = l_idx.value_or(INVALID_TEXTURE_INDEX);
				}
			}
		}
		l_materials.push_back(l_materialCB);

		if (l_materials.size() >= m_BuiltMeshCount) break;
	}

	if (l_materials.size() == m_BuiltMeshCount)
		g_Engine->Get<GPUBufferResourceService>()->Upload(m_MaterialBuffer, l_materials.data());
}
