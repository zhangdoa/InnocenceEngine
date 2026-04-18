#include "GPUPathTracerPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/DrawCallService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Component/MeshComponent.h"
#include "../../Engine/Component/MaterialComponent.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/LightDataService.h"

using namespace Inno;

bool GPUPathTracerPass::Setup(IServiceConfig* systemConfig)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderStage = ShaderStage::RayGen | ShaderStage::ClosestHit | ShaderStage::AnyHit | ShaderStage::Miss;

	// --- Ray Tracing SPC ---
	m_RayTracingSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("GPUPathTracerPass");
	m_RayTracingSPC->m_ShaderFilePaths.m_RayGenPath     = "GPUPathTracerRayGen.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_ClosestHitPath = "GPUPathTracerClosestHit.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_AnyHitPath     = "GPUPathTracerAnyHit.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_MissPath       = "GPUPathTracerMiss.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_ShadowMissPath = "GPUPathTracerShadowMiss.hlsl";

	// --- Ray Tracing Render Pass ---
	m_RayTracingRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("GPUPathTracerPass");

	auto l_rtDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_rtDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_rtDesc.m_RenderTargetCount = 0;
	l_rtDesc.m_UseRaytracing    = true;
	l_rtDesc.m_UseOutputMerger  = false;

	m_RayTracingRenderPassComp->m_RenderPassDesc = l_rtDesc;

	// Binding layout: b0=PerFrameCB, b1=FrameCountCB, b2=LightCountCB,
	//                 t0=TLAS, t1=MaterialBuffer, t2=MegaVB, t3=MegaIB,
	//                 t4=MeshOffsets, t5=PointLightBuffer, t6=SphereLightBuffer,
	//                 u0=AccumBuffer
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs.resize(11);

	// b0 - PerFrameCB (set 0, binding 0)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex   = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage       = m_ShaderStage;

	// b1 - FrameCountCB (set 0, binding 1)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex   = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage       = m_ShaderStage;

	// t0 - TLAS (set 1, binding 0)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex        = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUBufferUsage         = GPUBufferUsage::TLAS;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage            = m_ShaderStage;

	// t1 - MaterialBuffer (set 1, binding 1, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex        = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage            = m_ShaderStage;

	// t2 - MegaVertexBuffer (set 1, binding 2, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex        = 2;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage            = m_ShaderStage;

	// t3 - MegaIndexBuffer (set 1, binding 3, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex        = 3;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage            = m_ShaderStage;

	// t4 - MeshOffsets (set 1, binding 4, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex        = 4;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage            = m_ShaderStage;

	// u0 - AccumulationBuffer (set 2, binding 0, ReadWrite UAV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType        = GPUResourceType::Image;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex      = 2;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex        = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage           = TextureUsage::ComputeOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_ShaderStage            = m_ShaderStage;

	// b2 - LightCountCB (set 0, binding 2)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex   = 2;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_ShaderStage       = m_ShaderStage;

	// t5 - PointLightBuffer (set 1, binding 5, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex        = 5;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_ShaderStage            = m_ShaderStage;

	// t6 - SphereLightBuffer (set 1, binding 6, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex        = 6;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_ShaderStage            = m_ShaderStage;

	m_RayTracingRenderPassComp->m_ShaderProgram = m_RayTracingSPC;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	// --- Scene callbacks ---
	f_sceneLoadedCallback = [this]()
	{
		m_PendingGeometryRebuild = true;
		m_FrameCount = 1;
	};

	f_sceneUnloadingCallback = [this]()
	{
		// GPU is guaranteed idle here (WaitForGPUIdle called before unloading callbacks).
		// Release geometry buffers now so they aren't deleted mid-frame in RebuildGeometryBuffers.
		auto l_bufService = g_Engine->Get<GPUBufferResourceService>();

		if (m_MegaVertexBuffer)
		{
			l_bufService->Delete(m_MegaVertexBuffer);
			m_MegaVertexBuffer = nullptr;
		}
		if (m_MegaIndexBuffer)
		{
			l_bufService->Delete(m_MegaIndexBuffer);
			m_MegaIndexBuffer = nullptr;
		}
		if (m_MeshOffsetBuffer)
		{
			l_bufService->Delete(m_MeshOffsetBuffer);
			m_MeshOffsetBuffer = nullptr;
		}
		if (m_MaterialBuffer)
		{
			l_bufService->Delete(m_MaterialBuffer);
			m_MaterialBuffer = nullptr;
		}

		m_BuiltMeshCount = 0;
		m_FrameCount = 1;
		m_ObjectStatus = ObjectStatus::Suspended;
	};

	g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadedCallback);
	g_Engine->Get<SceneService>()->AddSceneUnloadingCallback(&f_sceneUnloadingCallback);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool GPUPathTracerPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_RayTracingSPC);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RayTracingRenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);



	// AccumulationBuffer: HDR RGBA float32, ComputeOnly UAV
	m_AccumulationBuffer = g_Engine->Get<TextureResourceService>()->Add("GPUPathTracerAccumBuffer");
	m_AccumulationBuffer->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
	m_AccumulationBuffer->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
	m_AccumulationBuffer->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::RGBA;
	m_AccumulationBuffer->m_TextureDesc.PixelDataType    = TexturePixelDataType::Float32;
	m_AccumulationBuffer->m_TextureDesc.Width            = l_resolution.x;
	m_AccumulationBuffer->m_TextureDesc.Height           = l_resolution.y;
	m_AccumulationBuffer->m_TextureDesc.DepthOrArraySize = 1;
	m_AccumulationBuffer->m_CPUAccessibility             = Accessibility::Immutable;
	m_AccumulationBuffer->m_GPUAccessibility             = Accessibility::ReadWrite;
	g_Engine->Get<TextureResourceService>()->Initialize(m_AccumulationBuffer);

	// FrameCountCB: single uint32
	m_FrameCountCB = g_Engine->Get<GPUBufferResourceService>()->Add("GPUPathTracerFrameCountCB");
	m_FrameCountCB->m_ElementCount      = 1;
	m_FrameCountCB->m_ElementSize       = sizeof(uint32_t);
	m_FrameCountCB->m_CPUAccessibility  = Accessibility::WriteOnly;
	m_FrameCountCB->m_GPUAccessibility  = Accessibility::ReadOnly;
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_FrameCountCB);

	// LightCountCB: two uint32 (point count, sphere count) + two padding uint32
	m_LightCountCB = g_Engine->Get<GPUBufferResourceService>()->Add("GPUPathTracerLightCountCB");
	m_LightCountCB->m_ElementCount      = 1;
	m_LightCountCB->m_ElementSize       = sizeof(PathTracerLightCountData);
	m_LightCountCB->m_CPUAccessibility  = Accessibility::WriteOnly;
	m_LightCountCB->m_GPUAccessibility  = Accessibility::ReadOnly;
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_LightCountCB);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool GPUPathTracerPass::Update()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	size_t l_currentMeshOwnerCount = l_meshStorage.AllOwners().size();

	if (l_currentMeshOwnerCount != m_BuiltMeshCount)
		m_PendingGeometryRebuild = true;

	if (m_PendingGeometryRebuild && AreMeshesGPUReady())
	{
		RebuildGeometryBuffers();
		m_PendingGeometryRebuild = false;
	}

	const bool l_geometryReady =
		m_MegaVertexBuffer && m_MegaVertexBuffer->m_ObjectStatus == ObjectStatus::Activated &&
		m_MegaIndexBuffer  && m_MegaIndexBuffer->m_ObjectStatus  == ObjectStatus::Activated &&
		m_MeshOffsetBuffer && m_MeshOffsetBuffer->m_ObjectStatus == ObjectStatus::Activated &&
		m_MaterialBuffer   && m_MaterialBuffer->m_ObjectStatus   == ObjectStatus::Activated;

	m_ObjectStatus = l_geometryReady ? ObjectStatus::Activated : ObjectStatus::Suspended;

	if (!l_geometryReady)
		return true;

	const auto& l_perFrameCB = g_Engine->Get<PerFrameDataService>()->GetPerFrameConstantBuffer();

	if (std::memcmp(&l_perFrameCB.v, &m_PrevViewMatrix, sizeof(Math::Mat4)) != 0)
	{
		m_FrameCount = 1;
		m_PrevViewMatrix = l_perFrameCB.v;
	}
	else
	{
		m_FrameCount++;
	}

	if (m_FrameCountCB && m_FrameCountCB->m_ObjectStatus == ObjectStatus::Activated)
	{
		g_Engine->Get<GPUBufferResourceService>()->Upload(m_FrameCountCB, &m_FrameCount);
	}

	if (m_LightCountCB && m_LightCountCB->m_ObjectStatus == ObjectStatus::Activated)
	{
		auto l_lightService = g_Engine->Get<LightDataService>();
		PathTracerLightCountData l_lightCounts;
		l_lightCounts.pointLightCount  = l_lightService->GetPointLightCount();
		l_lightCounts.sphereLightCount = l_lightService->GetSphereLightCount();
		l_lightCounts.pad0 = 0;
		l_lightCounts.pad1 = 0;
		g_Engine->Get<GPUBufferResourceService>()->Upload(m_LightCountCB, &l_lightCounts);
	}

	return true;
}

bool GPUPathTracerPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	if (m_MaterialBuffer)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_MaterialBuffer);
	if (m_MeshOffsetBuffer)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_MeshOffsetBuffer);
	if (m_MegaIndexBuffer)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_MegaIndexBuffer);
	if (m_MegaVertexBuffer)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_MegaVertexBuffer);

	if (m_FrameCountCB)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_FrameCountCB);
	if (m_LightCountCB)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_LightCountCB);
	if (m_AccumulationBuffer)
		g_Engine->Get<TextureResourceService>()->Delete(m_AccumulationBuffer);

	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RayTracingRenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_RayTracingSPC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus GPUPathTracerPass::GetStatus()
{
	return m_ObjectStatus;
}

bool GPUPathTracerPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RayTracingRenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_MegaVertexBuffer || m_MegaVertexBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_MegaIndexBuffer || m_MegaIndexBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_MeshOffsetBuffer || m_MeshOffsetBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_MaterialBuffer || m_MaterialBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_perFrameBuffer  = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_resolution      = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	// Graphics CL: transition textures to compute-writable states.
	// Must happen on Graphics because tracked state may include PIXEL_SHADER_RESOURCE
	// (set by swap chain presentation), which is invalid on compute command lists.
	l_fmService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Graphics);

	// Compute CL: bind and dispatch rays
	l_fmService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_perFrameBuffer,                 0);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_FrameCountCB,                   1);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, g_Engine->Get<GPUBufferResourceService>()->GetTLASBuffer(), 2);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MaterialBuffer,                   3);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MegaVertexBuffer,                4);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MegaIndexBuffer,                 5);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MeshOffsetBuffer,                6);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_AccumulationBuffer,              7);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_LightCountCB,                    8);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, g_Engine->Get<LightDataService>()->GetPointLightBuffer(),  9);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, g_Engine->Get<LightDataService>()->GetSphereLightBuffer(), 10);

	l_fmService->DispatchRays(m_RayTracingRenderPassComp, m_CommandListComp_Compute, l_resolution.x, l_resolution.y, 1);
	l_fmService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);
	l_fmService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

	return true;
}

RenderPassComponent* GPUPathTracerPass::GetRenderPassComp()
{
	return m_RayTracingRenderPassComp;
}

GPUResourceComponent* GPUPathTracerPass::GetResult()
{
	return m_AccumulationBuffer;
}

void GPUPathTracerPass::ResetAccumulation()
{
	m_FrameCount = 1;
}

void GPUPathTracerPass::RebuildGeometryBuffers()
{
	auto l_registry = g_Engine->Get<EntityRegistry>();

	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_meshOwners = l_meshStorage.AllOwners();

	if (l_meshOwners.empty())
		return;

	m_PendingVertices.clear();
	m_PendingIndices.clear();
	m_PendingOffsets.clear();
	m_PendingMaterials.clear();

	auto& l_vertices = m_PendingVertices;
	auto& l_indices  = m_PendingIndices;
	auto& l_offsets  = m_PendingOffsets;
	auto& l_materials = m_PendingMaterials;

	// Iterate by entity in the same order as UpdateRaytracingInstances (TLAS build)
	for (EntityID l_entity : l_meshOwners)
	{
		auto* l_mesh = l_registry->Get<MeshComponent>(l_entity);
		if (!l_mesh || !l_mesh->m_Asset.IsValid())
			continue;

		if (l_mesh->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		const auto* l_resource = AssetService::GetMeshAsset(l_mesh->m_Asset);
		if (!l_resource || l_resource->m_Residency != AssetResidency::Resident)
			continue;

		if (!l_resource->m_MappedMemory_VB || !l_resource->m_MappedMemory_IB)
			continue;

		const uint32_t l_vertexStride = l_resource->m_VertexBufferView.m_StrideInBytes;
		const uint32_t l_indexStride  = l_resource->m_IndexBufferView.m_StrideInBytes;

		if (l_vertexStride == 0 || l_indexStride == 0)
			continue;

		const uint32_t l_vertexCount = l_resource->m_VertexBufferView.m_SizeInBytes / l_vertexStride;
		const uint32_t l_indexCount  = l_resource->GetIndexCount();

		if (l_vertexCount == 0 || l_indexCount == 0)
			continue;

		MeshOffsetData l_offset = {};
		l_offset.m_VertexOffset = static_cast<uint32_t>(l_vertices.size());
		l_offset.m_IndexOffset  = static_cast<uint32_t>(l_indices.size());
		l_offset.m_VertexCount  = l_vertexCount;
		l_offset.m_IndexCount   = l_indexCount;
		l_offsets.push_back(l_offset);

		// Collect material for this entity (matching TLAS instance index).
		// Zero-init gives a matte-white Lambert default, which is a visible-but-not-obvious
		// sentinel. Entities reaching this point without a MaterialComponent or without a
		// resolvable MaterialAsset are logged once each so the silent default doesn't hide
		// scene-import gaps or teardown-order bugs.
		MaterialConstantBuffer l_materialCB = {};
		auto* l_matComp = l_registry->Get<MaterialComponent>(l_entity);
		if (l_matComp == nullptr)
		{
			if (m_WarnedMissingMaterial.insert(l_entity).second)
				Log(Warning, "GPUPathTracer: entity '", l_registry->GetName(l_entity),
					"' (TLAS instance ", l_materials.size(), ") has a MeshComponent but no "
					"MaterialComponent — falling back to default white Lambert.");
		}
		else
		{
			auto* l_matAsset = AssetService::GetMaterialAsset(l_matComp->m_Asset);
			if (l_matAsset == nullptr)
			{
				if (m_WarnedMissingMaterial.insert(l_entity).second)
					Log(Warning, "GPUPathTracer: entity '", l_registry->GetName(l_entity),
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
		l_materials.push_back(l_materialCB);

		const uint8_t* l_vbPtr = static_cast<const uint8_t*>(l_resource->m_MappedMemory_VB);
		for (uint32_t v = 0; v < l_vertexCount; v++)
		{
			const Vertex* l_vert = reinterpret_cast<const Vertex*>(l_vbPtr + static_cast<size_t>(v) * l_vertexStride);
			GPUPathTracerVertex l_ptVertex = {};
			l_ptVertex.posX  = l_vert->m_pos.x;
			l_ptVertex.posY  = l_vert->m_pos.y;
			l_ptVertex.posZ  = l_vert->m_pos.z;
			l_ptVertex.normX = l_vert->m_normal.x;
			l_ptVertex.normY = l_vert->m_normal.y;
			l_ptVertex.normZ = l_vert->m_normal.z;
			l_vertices.push_back(l_ptVertex);
		}

		const uint8_t* l_ibPtr = static_cast<const uint8_t*>(l_resource->m_MappedMemory_IB);
		for (uint32_t idx = 0; idx < l_indexCount; idx++)
		{
			uint32_t l_index = 0;
			if (l_indexStride == 2)
				l_index = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(l_ibPtr + static_cast<size_t>(idx) * l_indexStride));
			else
				l_index = *reinterpret_cast<const uint32_t*>(l_ibPtr + static_cast<size_t>(idx) * l_indexStride);
			l_indices.push_back(l_index);
		}
	}

	if (l_vertices.empty() || l_indices.empty())
	{
		Log(Warning, "GPUPathTracerPass: No mesh data available for geometry buffers.");
		return;
	}

	auto l_bufService = g_Engine->Get<GPUBufferResourceService>();

	// Delete old buffers
	if (m_MegaVertexBuffer)
	{
		l_bufService->Delete(m_MegaVertexBuffer);
		m_MegaVertexBuffer = nullptr;
	}
	if (m_MegaIndexBuffer)
	{
		l_bufService->Delete(m_MegaIndexBuffer);
		m_MegaIndexBuffer = nullptr;
	}
	if (m_MeshOffsetBuffer)
	{
		l_bufService->Delete(m_MeshOffsetBuffer);
		m_MeshOffsetBuffer = nullptr;
	}
	if (m_MaterialBuffer)
	{
		l_bufService->Delete(m_MaterialBuffer);
		m_MaterialBuffer = nullptr;
	}

	// Create and upload mega vertex buffer (ReadWrite for SRV descriptor table binding)
	m_MegaVertexBuffer = l_bufService->Add("GPUPathTracerMegaVB");
	m_MegaVertexBuffer->m_ElementCount     = l_vertices.size();
	m_MegaVertexBuffer->m_ElementSize      = sizeof(GPUPathTracerVertex);
	m_MegaVertexBuffer->m_CPUAccessibility = Accessibility::WriteOnly;
	m_MegaVertexBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
	m_MegaVertexBuffer->m_InitialData      = l_vertices.data();
	l_bufService->Initialize(m_MegaVertexBuffer);

	// Create and upload mega index buffer
	m_MegaIndexBuffer = l_bufService->Add("GPUPathTracerMegaIB");
	m_MegaIndexBuffer->m_ElementCount     = l_indices.size();
	m_MegaIndexBuffer->m_ElementSize      = sizeof(uint32_t);
	m_MegaIndexBuffer->m_CPUAccessibility = Accessibility::WriteOnly;
	m_MegaIndexBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
	m_MegaIndexBuffer->m_InitialData      = l_indices.data();
	l_bufService->Initialize(m_MegaIndexBuffer);

	// Create and upload mesh offset buffer
	m_MeshOffsetBuffer = l_bufService->Add("GPUPathTracerMeshOffsets");
	m_MeshOffsetBuffer->m_ElementCount     = l_offsets.size();
	m_MeshOffsetBuffer->m_ElementSize      = sizeof(MeshOffsetData);
	m_MeshOffsetBuffer->m_CPUAccessibility = Accessibility::WriteOnly;
	m_MeshOffsetBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
	m_MeshOffsetBuffer->m_InitialData      = l_offsets.data();
	l_bufService->Initialize(m_MeshOffsetBuffer);

	// Create and upload material buffer (indexed by TLAS instance)
	m_MaterialBuffer = l_bufService->Add("GPUPathTracerMaterialBuffer");
	m_MaterialBuffer->m_ElementCount     = l_materials.size();
	m_MaterialBuffer->m_ElementSize      = sizeof(MaterialConstantBuffer);
	m_MaterialBuffer->m_CPUAccessibility = Accessibility::WriteOnly;
	m_MaterialBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
	m_MaterialBuffer->m_InitialData      = l_materials.data();
	l_bufService->Initialize(m_MaterialBuffer);

	m_BuiltMeshCount = l_meshOwners.size();

	Log(Success, "GPUPathTracerPass: Geometry buffers rebuilt. Meshes: ", l_offsets.size(),
		" Vertices: ", l_vertices.size(), " Indices: ", l_indices.size());
}

bool GPUPathTracerPass::AreMeshesGPUReady()
{
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_meshes = l_meshStorage.All();

	if (l_meshes.empty())
		return false;

	for (const auto& l_mesh : l_meshes)
	{
		if (l_mesh.m_ObjectStatus != ObjectStatus::Activated)
			return false;

		const auto* l_resource = AssetService::GetMeshAsset(l_mesh.m_Asset);
		if (!l_resource || l_resource->m_Residency != AssetResidency::Resident)
			return false;
	}

	return true;
}
