#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"
#include "../../Engine/RenderGraph/RenderGraphEnumStrings.h"
using namespace Inno;

static RenderGraphDesc MakeTiledGraph()
{
	RenderGraphDesc l_graph;

	ResourceDesc l_res;
	l_res.m_Name = "TiledMask";
	l_res.m_Type = RenderGraphResourceType::Texture;
	l_res.m_SizeExpr = "tiled";
	l_res.m_TileSize = 8;
	l_res.m_TextureDesc.DepthOrArraySize = 1;
	l_res.m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	l_res.m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	l_res.m_TextureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_res.m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	l_res.m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_graph.m_Resources.push_back(l_res);

	PassNodeDesc l_pass;
	l_pass.m_Name = "TiledMaskPass";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_ShaderFilePaths.m_CSPath = "tiledMaskPass.comp";
	l_pass.m_Writes.push_back("TiledMask");
	l_pass.m_Dispatch.m_Mode = DispatchMode::TiledDispatch;
	l_pass.m_Dispatch.m_TileSize = 8;
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

static RenderGraphDesc MakeTiledArrayGraph()
{
	RenderGraphDesc l_graph;
	l_graph.m_Name = "TiledArrayGraph";

	ResourceDesc l_res;
	l_res.m_Name = "TiledArrayAtlas";
	l_res.m_Type = RenderGraphResourceType::Texture;
	l_res.m_SizeExpr = "tiledArray";
	l_res.m_TileSize = 8;
	l_res.m_TileArraySize = 3;
	l_res.m_TextureDesc.DepthOrArraySize = 1;
	l_res.m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	l_res.m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	l_res.m_TextureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_res.m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	l_res.m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_graph.m_Resources.push_back(l_res);

	PassNodeDesc l_pass;
	l_pass.m_Name = "TiledArrayPass";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_ShaderFilePaths.m_CSPath = "tiledArrayPass.comp";
	l_pass.m_Writes.push_back("TiledArrayAtlas");
	l_pass.m_Dispatch.m_Mode = DispatchMode::TiledDispatch;
	l_pass.m_Dispatch.m_TileSize = 8;
	l_pass.m_Dispatch.m_DispatchScale = 2;
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

void TestTiledResourceRoundTrip()
{
	TestRunner::StartTest("RenderGraph: tiled resource + TiledDispatch survives round-trip");

	RenderGraphDesc l_original = MakeTiledGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed =
		l_loaded.m_Resources.size() == 1 &&
		l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[0];
		passed = r.m_Name == "TiledMask" &&
			r.m_SizeExpr == "tiled" &&
			r.m_TileSize == 8;
	}

	if (passed)
	{
		const auto& p = l_loaded.m_Passes[0];
		passed = p.m_Name == "TiledMaskPass" &&
			p.m_Dispatch.m_Mode == DispatchMode::TiledDispatch &&
			p.m_Dispatch.m_TileSize == 8;
	}

	TestRunner::EndTest(passed);
}

void TestTiledArrayResourceRoundTrip()
{
	TestRunner::StartTest("RenderGraph: tiledArray resource + TileArraySize survives round-trip");

	RenderGraphDesc l_original = MakeTiledArrayGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed =
		l_loaded.m_Resources.size() == 1 &&
		l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[0];
		passed = r.m_Name == "TiledArrayAtlas" &&
			r.m_SizeExpr == "tiledArray" &&
			r.m_TileSize == 8 &&
			r.m_TileArraySize == 3;
	}

	TestRunner::EndTest(passed);
}

void TestTiledDispatchRoundTrip()
{
	TestRunner::StartTest("RenderGraph: TiledDispatch + DispatchScale round-trips");

	{
		RenderGraphDesc l_original = MakeTiledArrayGraph();
		json j;
		RenderGraphSerializer::to_json(j, l_original);

		RenderGraphDesc l_loaded;
		RenderGraphSerializer::from_json(j, l_loaded);

		bool passed = l_loaded.m_Passes.size() == 1;
		if (passed)
		{
			const auto& d = l_loaded.m_Passes[0].m_Dispatch;
			passed = d.m_Mode == DispatchMode::TiledDispatch &&
				d.m_TileSize == 8 &&
				d.m_DispatchScale == 2;
		}
		TestRunner::EndTest(passed);
	}

	{
		TestRunner::StartTest("RenderGraph: default DispatchScale=1 omits from JSON");
		json j_no_scale = json{
			{ "Mode", "TiledDispatch" },
			{ "X", 1u }, { "Y", 1u }, { "Z", 1u },
			{ "TileSize", 8u } };
		DispatchDesc l_d;
		l_d.m_Mode = Inno::RenderGraphEnumStrings::DispatchModeFromString(j_no_scale.value("Mode", std::string("Static")));
		l_d.m_TileSize = j_no_scale.value("TileSize", 0u);
		l_d.m_DispatchScale = j_no_scale.value("DispatchScale", 1u);

		TestRunner::EndTest(l_d.m_Mode == DispatchMode::TiledDispatch &&
			l_d.m_TileSize == 8 &&
			l_d.m_DispatchScale == 1);
	}
}

void RunRenderGraphSerializerTiledUnitTests()
{
	TestRunner::StartTestSuite("RenderGraphSerializerTiled");
	TestTiledResourceRoundTrip();
	TestTiledArrayResourceRoundTrip();
	TestTiledDispatchRoundTrip();
	TestRunner::EndTestSuite();
}
