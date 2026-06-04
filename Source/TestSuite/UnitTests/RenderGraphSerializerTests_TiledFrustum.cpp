#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

static RenderGraphDesc MakeTiledFrustumGraph()
{
	// Mirrors the TiledFrustumGenerationPass node: a Default-kernel compute pass
	// on the data-driven TiledTwoLevel dispatch mode that binds PerFrameCBuffer +
	// its imperatively owned DispatchParams cbuffer and writes its imperatively
	// owned frustum buffer (both imported by name; the graph never allocates
	// them). No state transitions — the pass has no graphics command list.
	RenderGraphDesc l_graph;
	l_graph.m_Name = "TiledFrustumGraph";

	ResourceDesc l_output;
	l_output.m_Name = "TiledFrustumGPUBuffer";
	l_output.m_Type = RenderGraphResourceType::Buffer;
	l_output.m_Imported = true;
	l_graph.m_Resources.push_back(l_output);

	ResourceDesc l_dispatchParams;
	l_dispatchParams.m_Name = "TiledFrustumDispatchParams";
	l_dispatchParams.m_Type = RenderGraphResourceType::Buffer;
	l_dispatchParams.m_Imported = true;
	l_graph.m_Resources.push_back(l_dispatchParams);

	PassNodeDesc l_pass;
	l_pass.m_Name = "TiledFrustumGenerationPass";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_ShaderFilePaths.m_CSPath = "tileFrustum.comp";
	l_pass.m_Reads.push_back("PerFrameCBuffer");
	l_pass.m_Reads.push_back("TiledFrustumDispatchParams");
	l_pass.m_Writes.push_back("TiledFrustumGPUBuffer");

	auto l_addBinding = [&](const char* resource, uint32_t set, uint32_t index,
		Accessibility bindAccess, Accessibility resAccess)
	{
		BindingDesc b;
		b.m_Resource = resource;
		b.m_GPUResourceType = GPUResourceType::Buffer;
		b.m_DescriptorSetIndex = set;
		b.m_DescriptorIndex = index;
		b.m_BindingAccessibility = bindAccess;
		b.m_ResourceAccessibility = resAccess;
		b.m_ShaderStage = ShaderStage::Compute;
		l_pass.m_Bindings.push_back(b);
	};

	l_addBinding("PerFrameCBuffer", 0, 0, Accessibility::ReadOnly, Accessibility::ReadOnly);
	l_addBinding("TiledFrustumDispatchParams", 0, 1, Accessibility::ReadOnly, Accessibility::ReadOnly);
	l_addBinding("TiledFrustumGPUBuffer", 1, 0, Accessibility::ReadWrite, Accessibility::ReadWrite);

	l_pass.m_Dispatch = { 1, 1, 1, DispatchMode::TiledTwoLevel, 16 };
	l_pass.m_OneShot = false;
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

void TestTiledFrustumNodeRoundTrip()
{
	TestRunner::StartTest("RenderGraph: TiledFrustum node (TiledTwoLevel dispatch + imported buffers + no transitions) round-trips");

	RenderGraphDesc l_original = MakeTiledFrustumGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = l_loaded.m_Resources.size() == 2 && l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[0];
		passed = r.m_Name == "TiledFrustumGPUBuffer" &&
			r.m_Type == RenderGraphResourceType::Buffer &&
			r.m_Imported == true;
	}

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[1];
		passed = r.m_Name == "TiledFrustumDispatchParams" &&
			r.m_Type == RenderGraphResourceType::Buffer &&
			r.m_Imported == true;
	}

	if (passed)
	{
		const auto& p = l_loaded.m_Passes[0];
		passed = p.m_Name == "TiledFrustumGenerationPass" &&
			p.m_Dispatch.m_Mode == DispatchMode::TiledTwoLevel && p.m_Dispatch.m_TileSize == 16 &&
			p.m_Queue == GPUEngineType::Compute &&
			std::string(p.m_ShaderFilePaths.m_CSPath.c_str()) == "tileFrustum.comp" &&
			p.m_Reads.size() == 2 && p.m_Reads[0] == "PerFrameCBuffer" && p.m_Reads[1] == "TiledFrustumDispatchParams" &&
			p.m_Writes.size() == 1 && p.m_Writes[0] == "TiledFrustumGPUBuffer" &&
			p.m_Bindings.size() == 3 &&
			p.m_Transitions.size() == 0;
	}

	if (passed)
	{
		// Bindings round-trip by array position (== DX12 root parameter), the
		// order the kernel binds them; assert resource/type/set/index/access per slot.
		const auto& b = l_loaded.m_Passes[0].m_Bindings;
		passed =
			b[0].m_Resource == "PerFrameCBuffer" && b[0].m_GPUResourceType == GPUResourceType::Buffer &&
			b[0].m_DescriptorSetIndex == 0 && b[0].m_DescriptorIndex == 0 &&
			b[0].m_BindingAccessibility == Accessibility::ReadOnly && b[0].m_ResourceAccessibility == Accessibility::ReadOnly &&
			b[1].m_Resource == "TiledFrustumDispatchParams" && b[1].m_GPUResourceType == GPUResourceType::Buffer &&
			b[1].m_DescriptorSetIndex == 0 && b[1].m_DescriptorIndex == 1 &&
			b[1].m_BindingAccessibility == Accessibility::ReadOnly && b[1].m_ResourceAccessibility == Accessibility::ReadOnly &&
			b[2].m_Resource == "TiledFrustumGPUBuffer" && b[2].m_GPUResourceType == GPUResourceType::Buffer &&
			b[2].m_DescriptorSetIndex == 1 && b[2].m_DescriptorIndex == 0 &&
			b[2].m_BindingAccessibility == Accessibility::ReadWrite && b[2].m_ResourceAccessibility == Accessibility::ReadWrite;
	}

	if (passed)
	{
		// An empty transition set is omitted from the serialized node entirely
		// (sparse key), so the loaded pass carries zero transitions.
		passed = j["Passes"][0].contains("Transitions") == false;
	}

	TestRunner::EndTest(passed);
}
