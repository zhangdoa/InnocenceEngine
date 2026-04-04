#include "GPUPathTracerPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/DrawCallService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Component/MeshComponent.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

bool GPUPathTracerPass::Setup(IServiceConfig* systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	m_ShaderStage = ShaderStage::RayGen | ShaderStage::ClosestHit | ShaderStage::AnyHit | ShaderStage::Miss;

	// --- Ray Tracing SPC ---
	m_RayTracingSPC = l_graphicsService->AddShaderProgramComponent("GPUPathTracerPass/");
	m_RayTracingSPC->m_ShaderFilePaths.m_RayGenPath     = "GPUPathTracerRayGen.hlsl/";
	m_RayTracingSPC->m_ShaderFilePaths.m_ClosestHitPath = "GPUPathTracerClosestHit.hlsl/";
	m_RayTracingSPC->m_ShaderFilePaths.m_AnyHitPath     = "GPUPathTracerAnyHit.hlsl/";
	m_RayTracingSPC->m_ShaderFilePaths.m_MissPath       = "GPUPathTracerMiss.hlsl/";
	m_RayTracingSPC->m_ShaderFilePaths.m_ShadowMissPath = "GPUPathTracerShadowMiss.hlsl/";

	// --- Ray Tracing Render Pass ---
	m_RayTracingRenderPassComp = l_graphicsService->AddRenderPassComponent("GPUPathTracerPass/");

	auto l_rtDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_rtDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_rtDesc.m_RenderTargetCount = 0;
	l_rtDesc.m_UseRaytracing    = true;
	l_rtDesc.m_UseOutputMerger  = false;

	m_RayTracingRenderPassComp->m_RenderPassDesc = l_rtDesc;

	// Binding layout: b0=PerFrameCB, b1=FrameCountCB, t0=TLAS, t1=MaterialBuffer,
	//                 t2=MegaVB, t3=MegaIB, t4=MeshOffsets, u0=AccumBuffer
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs.resize(8);

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

	m_RayTracingRenderPassComp->m_ShaderProgram = m_RayTracingSPC;

	m_CommandListComp_Graphics = l_graphicsService->AddCommandListComponent("GPUPathTracerPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = l_graphicsService->AddCommandListComponent("GPUPathTracerPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	// --- ToneMap SPC ---
	m_ToneMapSPC = l_graphicsService->AddShaderProgramComponent("GPUPathTracerToneMapPass/");
	m_ToneMapSPC->m_ShaderFilePaths.m_CSPath = "GPUPathTracerToneMap.comp/";

	// --- ToneMap Render Pass ---
	m_ToneMapRenderPassComp = l_graphicsService->AddRenderPassComponent("GPUPathTracerToneMapPass/");

	auto l_tmDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_tmDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_tmDesc.m_RenderTargetCount = 0;
	l_tmDesc.m_UseOutputMerger  = false;

	m_ToneMapRenderPassComp->m_RenderPassDesc = l_tmDesc;

	// Binding layout: b0=PerFrameCB, t0=AccumBuffer(read), u0=ToneMapOutput(write)
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs.resize(3);

	// b0 - PerFrameCB (set 0, binding 0)
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType   = GPUResourceType::Buffer;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex   = 0;

	// t0 - AccumulationBuffer read (set 1, binding 0)
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType   = GPUResourceType::Image;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex   = 0;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage      = TextureUsage::ComputeOnly;

	// u0 - ToneMapOutput write (set 2, binding 0)
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType        = GPUResourceType::Image;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 2;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex        = 0;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage           = TextureUsage::ComputeOnly;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility  = Accessibility::ReadWrite;

	m_ToneMapRenderPassComp->m_ShaderProgram = m_ToneMapSPC;

	m_ToneMapCommandList = l_graphicsService->AddCommandListComponent("GPUPathTracerToneMapPass/");
	m_ToneMapCommandList->m_Type = GPUEngineType::Compute;

	// --- Scene callbacks ---
	f_sceneLoadedCallback = [this]()
	{
		RebuildGeometryBuffers();
		m_FrameCount = 1;
	};

	f_sceneUnloadingCallback = [this]()
	{
		m_FrameCount = 1;
	};

	g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadedCallback);
	g_Engine->Get<SceneService>()->AddSceneUnloadingCallback(&f_sceneUnloadingCallback);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool GPUPathTracerPass::Initialize()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	l_graphicsService->Initialize(m_RayTracingSPC);
	l_graphicsService->Initialize(m_RayTracingRenderPassComp);
	l_graphicsService->Initialize(m_CommandListComp_Graphics);
	l_graphicsService->Initialize(m_CommandListComp_Compute);

	l_graphicsService->Initialize(m_ToneMapSPC);
	l_graphicsService->Initialize(m_ToneMapRenderPassComp);
	l_graphicsService->Initialize(m_ToneMapCommandList);

	// AccumulationBuffer: HDR RGBA float32, ComputeOnly UAV
	m_AccumulationBuffer = l_graphicsService->AddTextureComponent("GPUPathTracerAccumBuffer/");
	m_AccumulationBuffer->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
	m_AccumulationBuffer->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
	m_AccumulationBuffer->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::RGBA;
	m_AccumulationBuffer->m_TextureDesc.PixelDataType    = TexturePixelDataType::Float32;
	m_AccumulationBuffer->m_TextureDesc.Width            = l_resolution.x;
	m_AccumulationBuffer->m_TextureDesc.Height           = l_resolution.y;
	m_AccumulationBuffer->m_TextureDesc.DepthOrArraySize = 1;
	m_AccumulationBuffer->m_CPUAccessibility             = Accessibility::Immutable;
	m_AccumulationBuffer->m_GPUAccessibility             = Accessibility::ReadWrite;
	l_graphicsService->Initialize(m_AccumulationBuffer);

	// ToneMapOutput: LDR RGBA UByte, ComputeOnly UAV
	m_ToneMapOutput = l_graphicsService->AddTextureComponent("GPUPathTracerToneMapOutput/");
	m_ToneMapOutput->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
	m_ToneMapOutput->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
	m_ToneMapOutput->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::RGBA;
	m_ToneMapOutput->m_TextureDesc.PixelDataType    = TexturePixelDataType::UByte;
	m_ToneMapOutput->m_TextureDesc.Width            = l_resolution.x;
	m_ToneMapOutput->m_TextureDesc.Height           = l_resolution.y;
	m_ToneMapOutput->m_TextureDesc.DepthOrArraySize = 1;
	m_ToneMapOutput->m_CPUAccessibility             = Accessibility::Immutable;
	m_ToneMapOutput->m_GPUAccessibility             = Accessibility::ReadWrite;
	l_graphicsService->Initialize(m_ToneMapOutput);

	// FrameCountCB: single uint32
	m_FrameCountCB = l_graphicsService->AddGPUBufferComponent("GPUPathTracerFrameCountCB/");
	m_FrameCountCB->m_ElementCount      = 1;
	m_FrameCountCB->m_ElementSize       = sizeof(uint32_t);
	m_FrameCountCB->m_CPUAccessibility  = Accessibility::WriteOnly;
	m_FrameCountCB->m_GPUAccessibility  = Accessibility::ReadOnly;
	l_graphicsService->Initialize(m_FrameCountCB);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool GPUPathTracerPass::Update()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

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
		l_graphicsService->Upload(m_FrameCountCB, &m_FrameCount);
	}

	const bool l_geometryReady =
		m_MegaVertexBuffer && m_MegaVertexBuffer->m_ObjectStatus == ObjectStatus::Activated &&
		m_MegaIndexBuffer  && m_MegaIndexBuffer->m_ObjectStatus  == ObjectStatus::Activated &&
		m_MeshOffsetBuffer && m_MeshOffsetBuffer->m_ObjectStatus == ObjectStatus::Activated;

	m_ObjectStatus = l_geometryReady ? ObjectStatus::Activated : ObjectStatus::Suspended;

	return true;
}

bool GPUPathTracerPass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	if (m_MeshOffsetBuffer)
		l_graphicsService->Delete(m_MeshOffsetBuffer);
	if (m_MegaIndexBuffer)
		l_graphicsService->Delete(m_MegaIndexBuffer);
	if (m_MegaVertexBuffer)
		l_graphicsService->Delete(m_MegaVertexBuffer);

	if (m_FrameCountCB)
		l_graphicsService->Delete(m_FrameCountCB);
	if (m_ToneMapOutput)
		l_graphicsService->Delete(m_ToneMapOutput);
	if (m_AccumulationBuffer)
		l_graphicsService->Delete(m_AccumulationBuffer);

	l_graphicsService->Delete(m_ToneMapCommandList);
	l_graphicsService->Delete(m_ToneMapRenderPassComp);
	l_graphicsService->Delete(m_ToneMapSPC);

	l_graphicsService->Delete(m_CommandListComp_Compute);
	l_graphicsService->Delete(m_CommandListComp_Graphics);
	l_graphicsService->Delete(m_RayTracingRenderPassComp);
	l_graphicsService->Delete(m_RayTracingSPC);

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

	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_perFrameBuffer  = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_materialBuffer  = g_Engine->Get<DrawCallService>()->GetMaterialBuffer();
	auto l_resolution      = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	// Graphics CL: transition AccumulationBuffer to ReadWrite
	l_graphicsService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Graphics, 0);
	l_graphicsService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_graphicsService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Graphics);

	// Compute CL: bind and dispatch rays
	l_graphicsService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Compute, 0);
	l_graphicsService->BindRenderPassComponent(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

	l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_perFrameBuffer,                 0);
	l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_FrameCountCB,                   1);
	l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_graphicsService->GetTLASBuffer(), 2);
	l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_materialBuffer,                  3);
	l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MegaVertexBuffer,                4);
	l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MegaIndexBuffer,                 5);
	l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MeshOffsetBuffer,                6);
	l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_AccumulationBuffer,              7);

	l_graphicsService->DispatchRays(m_RayTracingRenderPassComp, m_CommandListComp_Compute, l_resolution.x, l_resolution.y, 1);
	l_graphicsService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

	// ToneMap CL
	l_graphicsService->CommandListBegin(m_ToneMapRenderPassComp, m_ToneMapCommandList, 0);
	l_graphicsService->TryToTransitState(m_AccumulationBuffer, m_ToneMapCommandList, Accessibility::ReadWrite, Accessibility::ReadOnly);
	l_graphicsService->TryToTransitState(m_ToneMapOutput, m_ToneMapCommandList, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_graphicsService->BindRenderPassComponent(m_ToneMapRenderPassComp, m_ToneMapCommandList);

	l_graphicsService->BindGPUResource(m_ToneMapRenderPassComp, m_ToneMapCommandList, ShaderStage::Compute, l_perFrameBuffer,    0);
	l_graphicsService->BindGPUResource(m_ToneMapRenderPassComp, m_ToneMapCommandList, ShaderStage::Compute, m_AccumulationBuffer, 1);
	l_graphicsService->BindGPUResource(m_ToneMapRenderPassComp, m_ToneMapCommandList, ShaderStage::Compute, m_ToneMapOutput,     2);

	const uint32_t l_tileSize = 8;
	uint32_t l_groupX = (l_resolution.x + l_tileSize - 1) / l_tileSize;
	uint32_t l_groupY = (l_resolution.y + l_tileSize - 1) / l_tileSize;
	l_graphicsService->Dispatch(m_ToneMapRenderPassComp, m_ToneMapCommandList, l_groupX, l_groupY, 1);
	l_graphicsService->CommandListEnd(m_ToneMapRenderPassComp, m_ToneMapCommandList);

	return true;
}

RenderPassComponent* GPUPathTracerPass::GetRenderPassComp()
{
	return m_RayTracingRenderPassComp;
}

GPUResourceComponent* GPUPathTracerPass::GetResult()
{
	return m_ToneMapOutput;
}

CommandListComponent* GPUPathTracerPass::GetToneMapCommandList()
{
	return m_ToneMapCommandList;
}

void GPUPathTracerPass::ResetAccumulation()
{
	m_FrameCount = 1;
}

void GPUPathTracerPass::RebuildGeometryBuffers()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_registry        = g_Engine->Get<EntityRegistry>();

	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_meshes = l_meshStorage.All();

	if (l_meshes.empty())
		return;

	std::vector<GPUPathTracerVertex> l_vertices;
	std::vector<uint32_t>            l_indices;
	std::vector<MeshOffsetData>      l_offsets;

	for (size_t i = 0; i < l_meshes.size(); i++)
	{
		const MeshComponent& l_mesh = l_meshes[i];

		if (l_mesh.m_ObjectStatus != ObjectStatus::Activated)
			continue;

		const auto* l_resource = l_graphicsService->GetMeshResource(l_mesh.m_GPUResource);
		if (!l_resource || l_resource->m_Status != ObjectStatus::Activated)
			continue;

		if (!l_resource->m_MappedMemory_VB || !l_resource->m_MappedMemory_IB)
			continue;

		const uint32_t l_vertexStride  = l_resource->m_VertexBufferView.m_StrideInBytes;
		const uint32_t l_indexStride   = l_resource->m_IndexBufferView.m_StrideInBytes;

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
			{
				l_index = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(l_ibPtr + static_cast<size_t>(idx) * l_indexStride));
			}
			else
			{
				l_index = *reinterpret_cast<const uint32_t*>(l_ibPtr + static_cast<size_t>(idx) * l_indexStride);
			}
			l_indices.push_back(l_index);
		}
	}

	if (l_vertices.empty() || l_indices.empty())
	{
		Log(Warning, "GPUPathTracerPass: No mesh data available for geometry buffers.");
		return;
	}

	// Delete old buffers
	if (m_MegaVertexBuffer)
	{
		l_graphicsService->Delete(m_MegaVertexBuffer);
		m_MegaVertexBuffer = nullptr;
	}
	if (m_MegaIndexBuffer)
	{
		l_graphicsService->Delete(m_MegaIndexBuffer);
		m_MegaIndexBuffer = nullptr;
	}
	if (m_MeshOffsetBuffer)
	{
		l_graphicsService->Delete(m_MeshOffsetBuffer);
		m_MeshOffsetBuffer = nullptr;
	}

	// Create and upload mega vertex buffer
	m_MegaVertexBuffer = l_graphicsService->AddGPUBufferComponent("GPUPathTracerMegaVB/");
	m_MegaVertexBuffer->m_ElementCount     = l_vertices.size();
	m_MegaVertexBuffer->m_ElementSize      = sizeof(GPUPathTracerVertex);
	m_MegaVertexBuffer->m_CPUAccessibility = Accessibility::WriteOnly;
	m_MegaVertexBuffer->m_GPUAccessibility = Accessibility::ReadOnly;
	m_MegaVertexBuffer->m_InitialData      = l_vertices.data();
	l_graphicsService->Initialize(m_MegaVertexBuffer);

	// Create and upload mega index buffer
	m_MegaIndexBuffer = l_graphicsService->AddGPUBufferComponent("GPUPathTracerMegaIB/");
	m_MegaIndexBuffer->m_ElementCount     = l_indices.size();
	m_MegaIndexBuffer->m_ElementSize      = sizeof(uint32_t);
	m_MegaIndexBuffer->m_CPUAccessibility = Accessibility::WriteOnly;
	m_MegaIndexBuffer->m_GPUAccessibility = Accessibility::ReadOnly;
	m_MegaIndexBuffer->m_InitialData      = l_indices.data();
	l_graphicsService->Initialize(m_MegaIndexBuffer);

	// Create and upload mesh offset buffer
	m_MeshOffsetBuffer = l_graphicsService->AddGPUBufferComponent("GPUPathTracerMeshOffsets/");
	m_MeshOffsetBuffer->m_ElementCount     = l_offsets.size();
	m_MeshOffsetBuffer->m_ElementSize      = sizeof(MeshOffsetData);
	m_MeshOffsetBuffer->m_CPUAccessibility = Accessibility::WriteOnly;
	m_MeshOffsetBuffer->m_GPUAccessibility = Accessibility::ReadOnly;
	m_MeshOffsetBuffer->m_InitialData      = l_offsets.data();
	l_graphicsService->Initialize(m_MeshOffsetBuffer);

	Log(Success, "GPUPathTracerPass: Geometry buffers rebuilt. Meshes: ", l_offsets.size(),
		" Vertices: ", l_vertices.size(), " Indices: ", l_indices.size());
}
