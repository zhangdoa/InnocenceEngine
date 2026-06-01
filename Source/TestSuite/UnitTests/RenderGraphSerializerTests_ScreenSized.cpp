#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

static RenderGraphDesc MakeScreenSizedGraph()
{
	// Mirrors SkyPass: a deferred screen-sized RT (Size = "screen") written by a
	// ScreenTile-kernel pass.
	RenderGraphDesc l_graph;
	l_graph.m_Name = "ScreenSizedGraph";

	ResourceDesc l_res;
	l_res.m_Name = "Sky Pass Result";
	l_res.m_Type = RenderGraphResourceType::Texture;
	l_res.m_SizeExpr = "screen";
	l_res.m_TextureDesc.DepthOrArraySize = 1;
	l_res.m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	l_res.m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	l_res.m_TextureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_res.m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	l_res.m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_graph.m_Resources.push_back(l_res);

	PassNodeDesc l_pass;
	l_pass.m_Name = "SkyPass";
	l_pass.m_Kernel = "ScreenTile";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_ShaderFilePaths.m_CSPath = "skyPass.comp";
	l_pass.m_Writes.push_back("Sky Pass Result");
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

void TestScreenSizedRoundTrip()
{
	TestRunner::StartTest("RenderGraph: screen-sized RT + ScreenTile kernel survives round-trip");

	RenderGraphDesc l_original = MakeScreenSizedGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = l_loaded.m_Resources.size() == 1 && l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[0];
		passed = r.m_Name == "Sky Pass Result" &&
			r.m_Type == RenderGraphResourceType::Texture &&
			r.m_SizeExpr == "screen" &&
			r.m_Imported == false;
	}

	if (passed)
		passed = l_loaded.m_Passes[0].m_Kernel == "ScreenTile";

	TestRunner::EndTest(passed);
}
