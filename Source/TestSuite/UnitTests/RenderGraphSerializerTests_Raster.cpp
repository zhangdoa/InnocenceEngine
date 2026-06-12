#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

static RenderGraphDesc MakeRasterGraph()
{
	// Mirrors OpaquePass: a graphics-queue node that draws into a 4-RT + depth
	// OutputMergerTarget via ExecuteIndirect. Exercises VS/PS shader paths, a
	// root-constant binding (no resource handle), and the raster pipeline block.
	RenderGraphDesc l_graph;
	l_graph.m_Name = "RasterGraph";

	PassNodeDesc l_pass;
	l_pass.m_Name = "OpaquePass";
	l_pass.m_Queue = GPUEngineType::Graphics;
	l_pass.m_ShaderFilePaths.m_VSPath = "opaqueGeometryProcessPass.vert";
	l_pass.m_ShaderFilePaths.m_PSPath = "opaqueGeometryProcessPass.frag";
	l_pass.m_Reads.push_back("OpaqueCullingPass/IndirectDrawCommandBuffer");
	l_pass.m_Writes.push_back("OpaquePass_RT_0");

	BindingDesc l_rootConstant;
	l_rootConstant.m_GPUResourceType = GPUResourceType::Buffer;
	l_rootConstant.m_IsRootConstant = true;
	l_rootConstant.m_SubresourceCount = 2;
	l_pass.m_Bindings.push_back(l_rootConstant);

	BindingDesc l_perFrame;
	l_perFrame.m_Resource = "PerFrameCBuffer";
	l_perFrame.m_GPUResourceType = GPUResourceType::Buffer;
	l_perFrame.m_DescriptorSetIndex = 0;
	l_perFrame.m_DescriptorIndex = 1;
	l_perFrame.m_ShaderStage = ShaderStage::Vertex;
	l_pass.m_Bindings.push_back(l_perFrame);

	l_pass.m_Raster.m_Enabled = true;
	l_pass.m_Raster.m_RenderTargetCount = 4;
	l_pass.m_Raster.m_UseDepthBuffer = true;
	l_pass.m_Raster.m_IndirectDraw = true;
	l_pass.m_Raster.m_DepthEnable = true;
	l_pass.m_Raster.m_DepthWrite = true;
	l_pass.m_Raster.m_DepthCompare = ComparisionFunction::LessEqual;
	l_pass.m_Raster.m_DepthClamp = true;
	l_pass.m_Raster.m_UseCulling = true;
	l_pass.m_Raster.m_CrossQueueExitToCommon = true;
	l_pass.m_Raster.m_IndirectArgsBuffer = "OpaqueCullingPass/IndirectDrawCommandBuffer";
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

void TestRasterNodeRoundTrip()
{
	TestRunner::StartTest("RenderGraph: raster node (VS/PS + root constant + raster pipeline block) round-trips");

	RenderGraphDesc l_original = MakeRasterGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& p = l_loaded.m_Passes[0];
		passed = p.m_Queue == GPUEngineType::Graphics &&
			std::string(p.m_ShaderFilePaths.m_VSPath.c_str()) == "opaqueGeometryProcessPass.vert" &&
			std::string(p.m_ShaderFilePaths.m_PSPath.c_str()) == "opaqueGeometryProcessPass.frag" &&
			p.m_Bindings.size() == 2 &&
			// root constant binding preserved, carries no resource
			p.m_Bindings[0].m_IsRootConstant == true &&
			p.m_Bindings[0].m_SubresourceCount == 2 &&
			p.m_Bindings[1].m_IsRootConstant == false &&
			p.m_Bindings[1].m_SubresourceCount == 1;
	}

	if (passed)
	{
		const auto& r = l_loaded.m_Passes[0].m_Raster;
		passed = r.m_Enabled == true &&
			r.m_RenderTargetCount == 4 &&
			r.m_UseDepthBuffer == true &&
			r.m_IndirectDraw == true &&
			r.m_DepthEnable == true &&
			r.m_DepthWrite == true &&
			r.m_DepthCompare == ComparisionFunction::LessEqual &&
			r.m_DepthClamp == true &&
			r.m_UseCulling == true &&
			r.m_CrossQueueExitToCommon == true &&
			r.m_IndirectArgsBuffer == "OpaqueCullingPass/IndirectDrawCommandBuffer";
	}

	TestRunner::EndTest(passed);
}
