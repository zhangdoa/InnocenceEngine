#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

namespace
{
	// Mirrors the live 18-binding LightPass node: ScreenTile compute, 3 light
	// CBs (b0-b2) + GBuffer/BRDF/SSAO/grid/index/cache/sun-shadow SRVs + TLAS
	// (t14) + sampler (s1) + 2 RW outputs (u0/u1); 15 reads, 13 transitions.
	// Light CBs declare ResourceAccess ReadOnly so they land as CBV (b-regs);
	// ReadWrite would map to SRV (t-regs) and collide with the GBuffer SRVs.
	RenderGraphDesc MakeLightPassGraph()
	{
		RenderGraphDesc l_graph;
		l_graph.m_Name = "LightPassGraph";

		PassNodeDesc l_pass;
		l_pass.m_Name = "LightPass";
		l_pass.m_Queue = GPUEngineType::Compute;
		l_pass.m_ShaderFilePaths.m_CSPath = "lightPass.comp";

		l_pass.m_Reads.push_back("PerFrameCBuffer");
		l_pass.m_Reads.push_back("PointLightCBuffer");
		l_pass.m_Reads.push_back("SphereLightCBuffer");
		l_pass.m_Reads.push_back("OpaquePass_RT_0");
		l_pass.m_Reads.push_back("OpaquePass_RT_1");
		l_pass.m_Reads.push_back("OpaquePass_RT_2");
		l_pass.m_Reads.push_back("OpaquePass_RT_3");
		l_pass.m_Reads.push_back("BRDF LUT");
		l_pass.m_Reads.push_back("BRDF MS LUT");
		l_pass.m_Reads.push_back("SSAO_Result");
		l_pass.m_Reads.push_back("LightCulling LightGrid");
		l_pass.m_Reads.push_back("LightIndexList");
		l_pass.m_Reads.push_back("Radiance Cache Result");
		l_pass.m_Reads.push_back("SunShadowRT_Visibility");
		l_pass.m_Reads.push_back("TLAS");

		l_pass.m_Writes.push_back("LightPass Luminance Result");
		l_pass.m_Writes.push_back("LightPass Illuminance Result");

		auto l_addBinding = [&](const char* resource, GPUResourceType type, uint32_t set,
			uint32_t index, Accessibility bindAccess, Accessibility resAccess,
			TextureUsage usage, GPUBufferUsage bufferUsage = GPUBufferUsage::Generic,
			ShaderStage stage = ShaderStage::Compute)
		{
			BindingDesc b;
			b.m_Resource = resource;
			b.m_GPUResourceType = type;
			b.m_DescriptorSetIndex = set;
			b.m_DescriptorIndex = index;
			b.m_BindingAccessibility = bindAccess;
			b.m_ResourceAccessibility = resAccess;
			b.m_TextureUsage = usage;
			b.m_GPUBufferUsage = bufferUsage;
			b.m_ShaderStage = stage;
			l_pass.m_Bindings.push_back(b);
		};

		// CBs (set 0) — ReadOnly resource access => CBV range (b-registers).
		l_addBinding("PerFrameCBuffer", GPUResourceType::Buffer, 0, 0, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::Invalid);
		l_addBinding("PointLightCBuffer", GPUResourceType::Buffer, 0, 1, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::Invalid);
		l_addBinding("SphereLightCBuffer", GPUResourceType::Buffer, 0, 2, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::Invalid);

		// SRV images + light-index buffer (set 1)
		l_addBinding("OpaquePass_RT_0", GPUResourceType::Image, 1, 0, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
		l_addBinding("OpaquePass_RT_1", GPUResourceType::Image, 1, 1, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
		l_addBinding("OpaquePass_RT_2", GPUResourceType::Image, 1, 2, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
		l_addBinding("OpaquePass_RT_3", GPUResourceType::Image, 1, 3, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
		l_addBinding("BRDF LUT", GPUResourceType::Image, 1, 4, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ComputeOnly);
		l_addBinding("BRDF MS LUT", GPUResourceType::Image, 1, 5, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ComputeOnly);
		l_addBinding("SSAO_Result", GPUResourceType::Image, 1, 6, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
		l_addBinding("LightCulling LightGrid", GPUResourceType::Image, 1, 8, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
		l_addBinding("LightIndexList", GPUResourceType::Buffer, 1, 9, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::Invalid);
		l_addBinding("Radiance Cache Result", GPUResourceType::Image, 1, 10, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
		l_addBinding("SunShadowRT_Visibility", GPUResourceType::Image, 1, 13, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ComputeOnly);
		l_addBinding("TLAS", GPUResourceType::Buffer, 1, 14, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::Invalid, GPUBufferUsage::TLAS);

		// Sampler (set 2)
		l_addBinding("LightPass/PointSampler", GPUResourceType::Sampler, 2, 1, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::Invalid);

		// UAV outputs (set 3)
		l_addBinding("LightPass Luminance Result", GPUResourceType::Image, 3, 0, Accessibility::ReadWrite, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
		l_addBinding("LightPass Illuminance Result", GPUResourceType::Image, 3, 1, Accessibility::ReadWrite, Accessibility::ReadWrite, TextureUsage::ColorAttachment);

		// Prepass: bring the touched state to its binding-time accessibility.
		auto l_addTransition = [&](const char* resource, Accessibility from, Accessibility to)
		{
			TransitionDesc t;
			t.m_Resource = resource;
			t.m_From = from;
			t.m_To = to;
			l_pass.m_Transitions.push_back(t);
		};

		l_addTransition("OpaquePass_RT_0", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("OpaquePass_RT_1", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("OpaquePass_RT_2", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("OpaquePass_RT_3", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("BRDF LUT", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("BRDF MS LUT", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("SSAO_Result", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("LightCulling LightGrid", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("LightIndexList", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("Radiance Cache Result", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("SunShadowRT_Visibility", Accessibility::WriteOnly, Accessibility::ReadOnly);
		l_addTransition("LightPass Luminance Result", Accessibility::ReadOnly, Accessibility::WriteOnly);
		l_addTransition("LightPass Illuminance Result", Accessibility::ReadOnly, Accessibility::WriteOnly);

		l_pass.m_Dispatch.m_Mode = DispatchMode::ScreenTile;
		l_pass.m_Dispatch.m_X = 1;
		l_pass.m_Dispatch.m_Y = 1;
		l_pass.m_Dispatch.m_Z = 1;
		l_pass.m_Dispatch.m_TileSize = 8;

		l_pass.m_BypassEnabled = false;
		l_pass.m_ClearOnBypass = false;
		l_pass.m_OneShot = false;

		l_graph.m_Passes.push_back(l_pass);
		return l_graph;
	}
}

void TestLightPassFullNodeRoundTrip()
{
	TestRunner::StartTest("RenderGraph: LightPass full node (18 bindings, 15 reads, 13 transitions, CBV light buffers + TLAS + sampler + RW outputs) round-trips");

	RenderGraphDesc l_original = MakeLightPassGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	// Sanity: on-the-wire JSON shape matches the contract.
	const auto& passJson = j.at("Passes").at(0);
	const auto& bindingsJson = passJson.at("Bindings");
	const auto& readsJson = passJson.at("Reads");
	const auto& transitionsJson = passJson.at("Transitions");
	bool jsonShapeOk = bindingsJson.is_array() && bindingsJson.size() == 18
		&& readsJson.is_array() && readsJson.size() == 15
		&& transitionsJson.is_array() && transitionsJson.size() == 13
		&& std::string(passJson.at("Shader").at("CS").get<std::string>().c_str()) == "lightPass.comp";

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = jsonShapeOk && l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& p = l_loaded.m_Passes[0];
		passed = std::string(p.m_Name.c_str()) == "LightPass"
			&& p.m_Queue == GPUEngineType::Compute
			&& std::string(p.m_ShaderFilePaths.m_CSPath.c_str()) == "lightPass.comp"
			&& p.m_Writes.size() == 2
			&& p.m_Writes[0] == "LightPass Luminance Result"
			&& p.m_Writes[1] == "LightPass Illuminance Result"
			&& p.m_Bindings.size() == 18
			&& p.m_Reads.size() == 15
			&& p.m_Transitions.size() == 13
			&& p.m_Dispatch.m_Mode == DispatchMode::ScreenTile
			&& p.m_Dispatch.m_TileSize == 8
			&& p.m_BypassEnabled == false
			&& p.m_ClearOnBypass == false
			&& p.m_OneShot == false;
	}

	// Reads preserve the exact contract order — the graph walks them in array
	// order to schedule prepass barriers.
	if (passed)
	{
		const auto& r = l_loaded.m_Passes[0].m_Reads;
		const char* expectedReads[15] = {
			"PerFrameCBuffer", "PointLightCBuffer", "SphereLightCBuffer",
			"OpaquePass_RT_0", "OpaquePass_RT_1", "OpaquePass_RT_2", "OpaquePass_RT_3",
			"BRDF LUT", "BRDF MS LUT", "SSAO_Result",
			"LightCulling LightGrid", "LightIndexList",
			"Radiance Cache Result", "SunShadowRT_Visibility", "TLAS"
		};
		for (uint32_t i = 0; i < 15; ++i)
		{
			if (r[i] != expectedReads[i]) { passed = false; break; }
		}
	}

	// Bindings round-trip by array position (== DX12 root parameter); assert
	// resource/type/set/index per slot so a stray reorder or missing entry
	// fails the test rather than compiling against the wrong register.
	if (passed)
	{
		const auto& b = l_loaded.m_Passes[0].m_Bindings;
		passed = passed
			// CBs
			&& b[0].m_Resource == "PerFrameCBuffer" && b[0].m_GPUResourceType == GPUResourceType::Buffer && b[0].m_DescriptorSetIndex == 0 && b[0].m_DescriptorIndex == 0
			&& b[1].m_Resource == "PointLightCBuffer" && b[1].m_DescriptorSetIndex == 0 && b[1].m_DescriptorIndex == 1
			&& b[2].m_Resource == "SphereLightCBuffer" && b[2].m_DescriptorSetIndex == 0 && b[2].m_DescriptorIndex == 2
			// SRV images
			&& b[3].m_Resource == "OpaquePass_RT_0" && b[3].m_GPUResourceType == GPUResourceType::Image && b[3].m_DescriptorSetIndex == 1 && b[3].m_DescriptorIndex == 0
			&& b[4].m_Resource == "OpaquePass_RT_1" && b[4].m_DescriptorSetIndex == 1 && b[4].m_DescriptorIndex == 1
			&& b[5].m_Resource == "OpaquePass_RT_2" && b[5].m_DescriptorSetIndex == 1 && b[5].m_DescriptorIndex == 2
			&& b[6].m_Resource == "OpaquePass_RT_3" && b[6].m_DescriptorSetIndex == 1 && b[6].m_DescriptorIndex == 3
			&& b[7].m_Resource == "BRDF LUT" && b[7].m_DescriptorSetIndex == 1 && b[7].m_DescriptorIndex == 4
			&& b[8].m_Resource == "BRDF MS LUT" && b[8].m_DescriptorSetIndex == 1 && b[8].m_DescriptorIndex == 5
			&& b[9].m_Resource == "SSAO_Result" && b[9].m_DescriptorSetIndex == 1 && b[9].m_DescriptorIndex == 6
			&& b[10].m_Resource == "LightCulling LightGrid" && b[10].m_DescriptorSetIndex == 1 && b[10].m_DescriptorIndex == 8
			&& b[11].m_Resource == "LightIndexList" && b[11].m_GPUResourceType == GPUResourceType::Buffer && b[11].m_DescriptorSetIndex == 1 && b[11].m_DescriptorIndex == 9
			&& b[12].m_Resource == "Radiance Cache Result" && b[12].m_DescriptorSetIndex == 1 && b[12].m_DescriptorIndex == 10
			&& b[13].m_Resource == "SunShadowRT_Visibility" && b[13].m_DescriptorSetIndex == 1 && b[13].m_DescriptorIndex == 13
			&& b[14].m_Resource == "TLAS" && b[14].m_GPUResourceType == GPUResourceType::Buffer && b[14].m_DescriptorSetIndex == 1 && b[14].m_DescriptorIndex == 14 && b[14].m_GPUBufferUsage == GPUBufferUsage::TLAS
			// Sampler
			&& b[15].m_Resource == "LightPass/PointSampler" && b[15].m_GPUResourceType == GPUResourceType::Sampler && b[15].m_DescriptorSetIndex == 2 && b[15].m_DescriptorIndex == 1
			// UAV outputs
			&& b[16].m_Resource == "LightPass Luminance Result" && b[16].m_DescriptorSetIndex == 3 && b[16].m_DescriptorIndex == 0
			&& b[17].m_Resource == "LightPass Illuminance Result" && b[17].m_DescriptorSetIndex == 3 && b[17].m_DescriptorIndex == 1;
	}

	// CB resource access lands the light buffers as CBVs (ReadOnly); UAV
	// outputs are ReadWrite on both binding and resource sides.
	if (passed)
	{
		const auto& b = l_loaded.m_Passes[0].m_Bindings;
		passed = b[0].m_ResourceAccessibility == Accessibility::ReadOnly
			&& b[1].m_ResourceAccessibility == Accessibility::ReadOnly
			&& b[2].m_ResourceAccessibility == Accessibility::ReadOnly
			&& b[16].m_BindingAccessibility == Accessibility::ReadWrite && b[16].m_ResourceAccessibility == Accessibility::ReadWrite
			&& b[17].m_BindingAccessibility == Accessibility::ReadWrite && b[17].m_ResourceAccessibility == Accessibility::ReadWrite;
	}

	// Transitions preserve the contract order so the prepass emits barriers in
	// the sequence the engine scheduler expects.
	if (passed)
	{
		const auto& t = l_loaded.m_Passes[0].m_Transitions;
		const char* expected[13] = {
			"OpaquePass_RT_0", "OpaquePass_RT_1", "OpaquePass_RT_2", "OpaquePass_RT_3",
			"BRDF LUT", "BRDF MS LUT", "SSAO_Result",
			"LightCulling LightGrid", "LightIndexList",
			"Radiance Cache Result", "SunShadowRT_Visibility",
			"LightPass Luminance Result", "LightPass Illuminance Result"
		};
		for (uint32_t i = 0; i < 13; ++i)
		{
			if (t[i].m_Resource != expected[i]) { passed = false; break; }
			if (i < 11 && (t[i].m_From != Accessibility::WriteOnly || t[i].m_To != Accessibility::ReadOnly)) { passed = false; break; }
			if (i >= 11 && (t[i].m_From != Accessibility::ReadOnly || t[i].m_To != Accessibility::WriteOnly)) { passed = false; break; }
		}
	}

	TestRunner::EndTest(passed);
}
