#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

static RenderGraphDesc MakeRaytracingGraph()
{
	// Mirrors SunShadowRTPass: a compute-queue raytracing node with RT shader
	// stages, a TLAS binding (root SRV), and a DispatchRays dispatch.
	RenderGraphDesc l_graph;
	l_graph.m_Name = "RaytracingGraph";

	PassNodeDesc l_pass;
	l_pass.m_Name = "SunShadowRTPass";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_UseRaytracing = true;
	l_pass.m_ShaderFilePaths.m_RayGenPath = "SunShadowRTRayGen.hlsl";
	l_pass.m_ShaderFilePaths.m_AnyHitPath = "SunShadowRTAnyHit.hlsl";
	l_pass.m_ShaderFilePaths.m_ClosestHitPath = "SunShadowRTClosestHit.hlsl";
	l_pass.m_ShaderFilePaths.m_MissPath = "SunShadowRTMiss.hlsl";
	l_pass.m_ShaderFilePaths.m_ShadowMissPath = "SunShadowRTShadowMiss.hlsl";
	l_pass.m_Reads.push_back("TLAS");
	l_pass.m_Writes.push_back("SunShadowRT_Visibility");

	BindingDesc l_tlas;
	l_tlas.m_Resource = "TLAS";
	l_tlas.m_GPUResourceType = GPUResourceType::Buffer;
	l_tlas.m_GPUBufferUsage = GPUBufferUsage::TLAS;
	l_tlas.m_DescriptorSetIndex = 1;
	l_tlas.m_DescriptorIndex = 0;
	l_pass.m_Bindings.push_back(l_tlas);

	l_pass.m_Dispatch.m_Mode = DispatchMode::DispatchRays;
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

void TestRaytracingNodeRoundTrip()
{
	TestRunner::StartTest("RenderGraph: raytracing node (RT shaders + TLAS binding + DispatchRays) round-trips");

	RenderGraphDesc l_original = MakeRaytracingGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& p = l_loaded.m_Passes[0];
		passed = p.m_UseRaytracing == true &&
			p.m_Dispatch.m_Mode == DispatchMode::DispatchRays &&
			std::string(p.m_ShaderFilePaths.m_RayGenPath.c_str()) == "SunShadowRTRayGen.hlsl" &&
			std::string(p.m_ShaderFilePaths.m_AnyHitPath.c_str()) == "SunShadowRTAnyHit.hlsl" &&
			std::string(p.m_ShaderFilePaths.m_ClosestHitPath.c_str()) == "SunShadowRTClosestHit.hlsl" &&
			std::string(p.m_ShaderFilePaths.m_MissPath.c_str()) == "SunShadowRTMiss.hlsl" &&
			std::string(p.m_ShaderFilePaths.m_ShadowMissPath.c_str()) == "SunShadowRTShadowMiss.hlsl";
	}

	if (passed)
	{
		const auto& b = l_loaded.m_Passes[0].m_Bindings[0];
		passed = l_loaded.m_Passes[0].m_Bindings.size() == 1 &&
			b.m_GPUBufferUsage == GPUBufferUsage::TLAS &&
			b.m_Resource == "TLAS";
	}

	TestRunner::EndTest(passed);
}
