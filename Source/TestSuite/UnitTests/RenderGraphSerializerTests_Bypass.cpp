#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

namespace
{
	// Minimal graph exercising the Bypass.Enabled primitive: one live compute
	// pass (writes "Live Result"), and one bypassed raster pass (writes a
	// screen-sized texture "Dead Result" that must round-trip but should never
	// be instantiated at runtime). Bypassed node reuses the same dispatch
	// surface (Raster + screen Size) as a working raster pass — exercises the
	// bypass-skip path for both resource allocation and CreatePassNode.
	RenderGraphDesc MakeBypassGraph()
	{
		RenderGraphDesc l_graph;
		l_graph.m_Name = "BypassGraph";

		// Live compute pass — its result is read by the bypassed pass.
		PassNodeDesc l_live;
		l_live.m_Name = "LivePass";
		l_live.m_Queue = GPUEngineType::Compute;
		l_live.m_ShaderFilePaths.m_CSPath = "livePass.comp";
		l_live.m_Writes.push_back("Live Result");

		BindingDesc l_liveOut;
		l_liveOut.m_Resource = "Live Result";
		l_liveOut.m_GPUResourceType = GPUResourceType::Image;
		l_liveOut.m_DescriptorSetIndex = 0;
		l_liveOut.m_DescriptorIndex = 0;
		l_liveOut.m_BindingAccessibility = Accessibility::ReadWrite;
		l_liveOut.m_ResourceAccessibility = Accessibility::ReadWrite;
		l_liveOut.m_TextureUsage = TextureUsage::ComputeOnly;
		l_liveOut.m_ShaderStage = ShaderStage::Compute;
		l_live.m_Bindings.push_back(l_liveOut);

		l_live.m_Dispatch.m_Mode = DispatchMode::ScreenTile;
		l_live.m_Dispatch.m_TileSize = 8;
		l_graph.m_Passes.push_back(l_live);

		// Bypassed raster pass — exercises the raster-pipeline + screen-size
		// description paths through the round-trip without going live.
		PassNodeDesc l_bypassed;
		l_bypassed.m_Name = "DeadPass";
		l_bypassed.m_Queue = GPUEngineType::Graphics;
		l_bypassed.m_ShaderFilePaths.m_VSPath = "deadPass.vert";
		l_bypassed.m_ShaderFilePaths.m_PSPath = "deadPass.frag";
		l_bypassed.m_BypassEnabled = true;
		l_bypassed.m_ClearOnBypass = true;
		l_bypassed.m_Reads.push_back("Live Result");
		l_bypassed.m_Writes.push_back("Dead Result");

		BindingDesc l_bypassedIn;
		l_bypassedIn.m_Resource = "Live Result";
		l_bypassedIn.m_GPUResourceType = GPUResourceType::Image;
		l_bypassedIn.m_DescriptorSetIndex = 0;
		l_bypassedIn.m_DescriptorIndex = 0;
		l_bypassedIn.m_BindingAccessibility = Accessibility::ReadOnly;
		l_bypassedIn.m_ResourceAccessibility = Accessibility::ReadOnly;
		l_bypassedIn.m_TextureUsage = TextureUsage::ComputeOnly;
		l_bypassedIn.m_ShaderStage = ShaderStage::Pixel;
		l_bypassed.m_Bindings.push_back(l_bypassedIn);

		l_bypassed.m_Raster.m_Enabled = true;
		l_bypassed.m_Raster.m_RenderTargetCount = 1;
		l_bypassed.m_Raster.m_UseDepthBuffer = true;
		l_bypassed.m_Raster.m_DepthEnable = true;
		l_bypassed.m_Raster.m_DepthWrite = true;
		l_bypassed.m_Raster.m_DepthCompare = ComparisionFunction::LessEqual;
		l_bypassed.m_Raster.m_IndirectDraw = true;
		l_bypassed.m_Raster.m_IndirectArgsBuffer = "LiveResult/IndirectArgs";
		l_bypassed.m_Raster.m_CrossQueueExitToCommon = true;
		l_graph.m_Passes.push_back(l_bypassed);

		return l_graph;
	}
}

void TestBypassNodeRoundTrip()
{
	TestRunner::StartTest("RenderGraph: bypass node (Bypass.Enabled + Bypass.ClearOnBypass + raster) round-trips");

	RenderGraphDesc l_original = MakeBypassGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	// Sanity: the on-the-wire JSON should have a Bypass object with both
	// Enabled and ClearOnBypass set, distinguishing the bypassed pass from the
	// live one (which carries the default-false Bypass omitted / serialised as
	// both false in the always-present Bypass object).
	const auto& passes = j.at("Passes");
	bool jsonShapeOk = passes.is_array() && passes.size() == 2
		&& passes[0].contains("Bypass")
		&& passes[0]["Bypass"].value("Enabled", true) == false
		&& passes[0]["Bypass"].value("ClearOnBypass", true) == false
		&& passes[1].contains("Bypass")
		&& passes[1]["Bypass"].value("Enabled", false) == true
		&& passes[1]["Bypass"].value("ClearOnBypass", false) == true;

	bool passed = jsonShapeOk;

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	if (passed)
	{
		passed = l_loaded.m_Passes.size() == 2;
	}

	if (passed)
	{
		// Live pass: bypass stays false (default).
		const auto& l_live = l_loaded.m_Passes[0];
		passed = std::string(l_live.m_Name.c_str()) == "LivePass"
			&& l_live.m_BypassEnabled == false
			&& l_live.m_ClearOnBypass == false
			&& l_live.m_Raster.m_Enabled == false;
	}

	if (passed)
	{
		// Bypassed pass: bypass flags preserved; raster block round-trips.
		const auto& l_dead = l_loaded.m_Passes[1];
		passed = std::string(l_dead.m_Name.c_str()) == "DeadPass"
			&& l_dead.m_BypassEnabled == true
			&& l_dead.m_ClearOnBypass == true
			&& l_dead.m_Queue == GPUEngineType::Graphics
			&& std::string(l_dead.m_ShaderFilePaths.m_VSPath.c_str()) == "deadPass.vert"
			&& std::string(l_dead.m_ShaderFilePaths.m_PSPath.c_str()) == "deadPass.frag"
			&& l_dead.m_Bindings.size() == 1
			&& l_dead.m_Bindings[0].m_Resource == "Live Result";
	}

	if (passed)
	{
		const auto& l_raster = l_loaded.m_Passes[1].m_Raster;
		passed = l_raster.m_Enabled == true
			&& l_raster.m_RenderTargetCount == 1
			&& l_raster.m_UseDepthBuffer == true
			&& l_raster.m_IndirectDraw == true
			&& l_raster.m_DepthEnable == true
			&& l_raster.m_DepthWrite == true
			&& l_raster.m_DepthCompare == ComparisionFunction::LessEqual
			&& l_raster.m_CrossQueueExitToCommon == true
			&& l_raster.m_IndirectArgsBuffer == "LiveResult/IndirectArgs";
	}

	TestRunner::EndTest(passed);
}
