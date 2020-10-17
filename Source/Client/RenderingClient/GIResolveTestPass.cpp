#include "GIResolveTestPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "GIResolvePass.h"
#include "OpaquePass.h"
#include "GIDataLoader.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

struct ProbePos
{
	Vec4 pos;
	Vec4 index;
};

struct ProbeMeshData
{
	Mat4 m;
	Vec4 index;
	float padding[12];
};

namespace GIResolveTestPass
{
	GPUBufferDataComponent* m_probeSphereMeshGBDC = 0;

	RenderPassDataComponent* m_probeRPDC = 0;
	ShaderProgramComponent* m_probeSPC = 0;
	SamplerDataComponent* m_SDC = 0;

	std::vector<ProbeMeshData> m_probeSphereMeshData;
}

bool GIResolveTestPass::Setup()
{
	m_probeSphereMeshGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("ProbeSphereMeshGPUBuffer/");
	m_probeSphereMeshGBDC->m_ElementCount = 4096;
	m_probeSphereMeshGBDC->m_ElementSize = sizeof(ProbeMeshData);
	m_probeSphereMeshGBDC->m_GPUAccessibility = Accessibility::ReadWrite;

	m_probeSphereMeshData.reserve(4096);

	////
	m_probeSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveTestProbePass/");

	m_probeSPC->m_ShaderFilePaths.m_VSPath = "GIResolveTestProbePass.vert/";
	m_probeSPC->m_ShaderFilePaths.m_PSPath = "GIResolveTestProbePass.frag/";

	m_probeRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("GIResolveTestProbePass/");

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

	m_probeRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_probeRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 8;

	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadOnly;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 1;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_probeRPDC->m_ShaderProgram = m_probeSPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("GIResolveTestProbePass/");

	return true;
}

bool GIResolveTestPass::Initialize()
{
	m_probeRPDC->m_DepthStencilRenderTarget = OpaquePass::GetRPDC()->m_DepthStencilRenderTarget;

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_probeSphereMeshGBDC);
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_probeSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_probeRPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool GIResolveTestPass::Render()
{
	auto l_probes = GIDataLoader::GetProbes();

	if (l_probes.size() > 0)
	{
		auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
		auto l_GIGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GI);

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

		auto l_sphere = g_Engine->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Sphere);

		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_probeSphereMeshGBDC, m_probeSphereMeshData, 0, m_probeSphereMeshData.size());

		g_Engine->getRenderingServer()->CommandListBegin(m_probeRPDC, 0);
		g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_probeRPDC);
		g_Engine->getRenderingServer()->CleanRenderTargets(m_probeRPDC);

		g_Engine->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 4, 0);

		g_Engine->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
		g_Engine->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Pixel, l_GIGBDC->m_ResourceBinder, 1, 8, Accessibility::ReadOnly);
		g_Engine->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Vertex, m_probeSphereMeshGBDC->m_ResourceBinder, 2, 0, Accessibility::ReadOnly);
		g_Engine->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Pixel, GIResolvePass::GetProbeVolume(), 3, 1, Accessibility::ReadOnly);

		g_Engine->getRenderingServer()->DispatchDrawCall(m_probeRPDC, l_sphere, m_probeSphereMeshData.size());

		g_Engine->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Pixel, GIResolvePass::GetProbeVolume(), 3, 1, Accessibility::ReadOnly);

		g_Engine->getRenderingServer()->CommandListEnd(m_probeRPDC);

		g_Engine->getRenderingServer()->ExecuteCommandList(m_probeRPDC);

		g_Engine->getRenderingServer()->WaitForFrame(m_probeRPDC);
	}

	return true;
}

bool GIResolveTestPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_probeRPDC);

	return true;
}

RenderPassDataComponent* GIResolveTestPass::GetRPDC()
{
	return m_probeRPDC;
}

ShaderProgramComponent* GIResolveTestPass::GetSPC()
{
	return m_probeSPC;
}