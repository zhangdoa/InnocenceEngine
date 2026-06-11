#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

static RenderGraphDesc MakePingPongGraph()
{
	// Mirrors TAAPass: a frame-parity ping-pong RT (m_PingPong) read back as
	// history (m_PingPongHistory on a binding + its transition) and written as
	// the current-frame output.
	RenderGraphDesc l_graph;
	l_graph.m_Name = "PingPongGraph";

	ResourceDesc l_res;
	l_res.m_Name = "TAA Pass Result";
	l_res.m_Type = RenderGraphResourceType::Texture;
	l_res.m_SizeExpr = "screen";
	l_res.m_PingPong = true;
	l_res.m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	l_res.m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	l_res.m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_graph.m_Resources.push_back(l_res);

	PassNodeDesc l_pass;
	l_pass.m_Name = "TAAPass";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_ShaderFilePaths.m_CSPath = "TAAPass.comp";
	l_pass.m_Writes.push_back("TAA Pass Result");

	BindingDesc l_history;
	l_history.m_Resource = "TAA Pass Result";
	l_history.m_GPUResourceType = GPUResourceType::Image;
	l_history.m_DescriptorSetIndex = 1;
	l_history.m_DescriptorIndex = 1;
	l_history.m_TextureUsage = TextureUsage::ColorAttachment;
	l_history.m_PingPongHistory = true;
	l_pass.m_Bindings.push_back(l_history);

	BindingDesc l_write;
	l_write.m_Resource = "TAA Pass Result";
	l_write.m_GPUResourceType = GPUResourceType::Image;
	l_write.m_DescriptorSetIndex = 2;
	l_write.m_DescriptorIndex = 0;
	l_write.m_BindingAccessibility = Accessibility::ReadWrite;
	l_write.m_ResourceAccessibility = Accessibility::ReadWrite;
	l_write.m_TextureUsage = TextureUsage::ComputeOnly;
	l_pass.m_Bindings.push_back(l_write);

	TransitionDesc l_historyTransition;
	l_historyTransition.m_Resource = "TAA Pass Result";
	l_historyTransition.m_From = Accessibility::WriteOnly;
	l_historyTransition.m_To = Accessibility::ReadOnly;
	l_historyTransition.m_PingPongHistory = true;
	l_pass.m_Transitions.push_back(l_historyTransition);

	TransitionDesc l_writeTransition;
	l_writeTransition.m_Resource = "TAA Pass Result";
	l_writeTransition.m_From = Accessibility::ReadOnly;
	l_writeTransition.m_To = Accessibility::WriteOnly;
	l_pass.m_Transitions.push_back(l_writeTransition);

	l_pass.m_Dispatch.m_Mode = DispatchMode::ScreenTile;
	l_pass.m_Dispatch.m_TileSize = 8;
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

void TestPingPongNodeRoundTrip()
{
	TestRunner::StartTest("RenderGraph: ping-pong resource + history binding/transition round-trips");

	RenderGraphDesc l_original = MakePingPongGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = l_loaded.m_Resources.size() == 1 && l_loaded.m_Passes.size() == 1;

	if (passed)
		passed = l_loaded.m_Resources[0].m_PingPong == true &&
			l_loaded.m_Resources[0].m_SizeExpr == "screen";

	if (passed)
	{
		const auto& p = l_loaded.m_Passes[0];
		passed = p.m_Bindings.size() == 2 && p.m_Transitions.size() == 2 &&
			// history binding flagged, write binding not
			p.m_Bindings[0].m_PingPongHistory == true &&
			p.m_Bindings[1].m_PingPongHistory == false &&
			// history transition flagged, write transition not
			p.m_Transitions[0].m_PingPongHistory == true &&
			p.m_Transitions[1].m_PingPongHistory == false;
	}

	TestRunner::EndTest(passed);
}
