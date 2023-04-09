#include "SurfelGITestPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "GIResolvePass.h"
#include "OpaquePass.h"
#include "GIDataLoader.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool SurfelGITestPass::Setup(ISystemConfig *systemConfig)
{
	m_probeSphereMeshGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("ProbeSphereMeshGPUBuffer/");
	m_probeSphereMeshGPUBufferComp->m_ElementCount = 4096;
	m_probeSphereMeshGPUBufferComp->m_ElementSize = sizeof(ProbeMeshData);
	m_probeSphereMeshGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_probeSphereMeshData.reserve(4096);

	////
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("SurfelGITestPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "GIResolveTestProbePass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "GIResolveTestProbePass.frag/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("SurfelGITestPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_UseStencilBuffer = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 8;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_SamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("SurfelGITestPass/");
	
	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SurfelGITestPass::Initialize()
{
	m_RenderPassComp->m_DepthStencilRenderTarget = OpaquePass::Get().GetRenderPassComp()->m_DepthStencilRenderTarget;

	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_probeSphereMeshGPUBufferComp);
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

ObjectStatus SurfelGITestPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SurfelGITestPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_probes = GIDataLoader::GetProbes();

	if (l_probes.size() > 0)
	{
		auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
		auto l_GIGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::GI);

		m_probeSphereMeshData.clear();

		std::vector<ProbePos> l_probePos;
		l_probePos.resize(l_probes.size());

		for (size_t i = 0; i < l_probes.size(); i++)
		{
			l_probePos[i].pos = l_probes[i].pos;
		}

		TVec4<uint32_t> l_probeIndex;
		float l_minPos;

		std::sort(l_probePos.begin(), l_probePos.end(), [&](ProbePos A, ProbePos B)
			{
				return A.pos.x < B.pos.x;
			});

		l_minPos = l_probePos[0].pos.x;

		for (size_t i = 0; i < l_probePos.size(); i++)
		{
			auto l_probePosWS = l_probePos[i].pos;

			if ((l_probePosWS.x - l_minPos) > epsilon<float, 2>)
			{
				l_minPos = l_probePosWS.x;
				l_probeIndex.x++;
			}

			l_probePos[i].index.x = (float)l_probeIndex.x;
		}

		std::sort(l_probePos.begin(), l_probePos.end(), [&](ProbePos A, ProbePos B)
			{
				return A.pos.y < B.pos.y;
			});

		l_minPos = l_probePos[0].pos.y;

		for (size_t i = 0; i < l_probePos.size(); i++)
		{
			auto l_probePosWS = l_probePos[i].pos;

			if ((l_probePosWS.y - l_minPos) > epsilon<float, 2>)
			{
				l_minPos = l_probePosWS.y;
				l_probeIndex.y++;
			}

			l_probePos[i].index.y = (float)l_probeIndex.y;
		}

		std::sort(l_probePos.begin(), l_probePos.end(), [&](ProbePos A, ProbePos B)
			{
				return A.pos.z < B.pos.z;
			});

		l_minPos = l_probePos[0].pos.z;

		for (size_t i = 0; i < l_probePos.size(); i++)
		{
			auto l_probePosWS = l_probePos[i].pos;

			if ((l_probePosWS.z - l_minPos) > epsilon<float, 2>)
			{
				l_minPos = l_probePosWS.z;
				l_probeIndex.z++;
			}

			l_probePos[i].index.z = (float)l_probeIndex.z;
		}

		for (size_t i = 0; i < l_probePos.size(); i++)
		{
			ProbeMeshData l_probeMeshData;

			l_probeMeshData.m = InnoMath::toTranslationMatrix(l_probePos[i].pos);
			l_probeMeshData.m.m00 *= 0.5f;
			l_probeMeshData.m.m11 *= 0.5f;
			l_probeMeshData.m.m22 *= 0.5f;
			l_probeMeshData.index = l_probePos[i].index;

			m_probeSphereMeshData.emplace_back(l_probeMeshData);
		}

		auto l_sphere = g_Engine->getRenderingFrontend()->getMeshComponent(ProceduralMeshShape::Sphere);

		g_Engine->getRenderingServer()->UploadGPUBufferComponent(m_probeSphereMeshGPUBufferComp, m_probeSphereMeshData, 0, m_probeSphereMeshData.size());

		g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
		g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
		g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_SamplerComp, 4);

		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_GIGPUBufferComp, 1, Accessibility::ReadOnly);
		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, m_probeSphereMeshGPUBufferComp, 2, Accessibility::ReadOnly);
		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, GIResolvePass::GetProbeVolume(), 3, Accessibility::ReadOnly);

		g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp, l_sphere, m_probeSphereMeshData.size());

		g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, GIResolvePass::GetProbeVolume(), 3, Accessibility::ReadOnly);

		g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

		g_Engine->getRenderingServer()->ExecuteCommandList(m_RenderPassComp, GPUEngineType::Graphics);

		
	}

	return true;
}

bool SurfelGITestPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

RenderPassComponent* SurfelGITestPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}