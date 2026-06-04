#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

static RenderGraphDesc MakeSSAOGraph()
{
	// Mirrors the SSAONoisePass node: a ScreenTile compute pass that binds two
	// imported samplers plus imported GBuffers/kernel/noise (set/index by array
	// position), transitions the noise texture and its own deferred screen-sized
	// Result on the graphics queue, and writes that Result.
	RenderGraphDesc l_graph;
	l_graph.m_Name = "SSAOGraph";

	ResourceDesc l_result;
	l_result.m_Name = "SSAO_Result";
	l_result.m_Type = RenderGraphResourceType::Texture;
	l_result.m_SizeExpr = "screen";
	l_result.m_TextureDesc.DepthOrArraySize = 1;
	l_result.m_TextureDesc.MipLevels = 1;
	l_result.m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	l_result.m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	l_result.m_TextureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_result.m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	l_result.m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_graph.m_Resources.push_back(l_result);

	ResourceDesc l_kernel;
	l_kernel.m_Name = "SSAO_Kernel";
	l_kernel.m_Type = RenderGraphResourceType::Buffer;
	l_kernel.m_Imported = true;
	l_graph.m_Resources.push_back(l_kernel);

	PassNodeDesc l_pass;
	l_pass.m_Name = "SSAONoisePass";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_ShaderFilePaths.m_CSPath = "SSAONoisePass.comp";
	l_pass.m_Reads.push_back("PerFrameCBuffer");
	l_pass.m_Reads.push_back("SSAO_Kernel");
	l_pass.m_Reads.push_back("OpaquePass_RT_0");
	l_pass.m_Reads.push_back("OpaquePass_RT_1");
	l_pass.m_Reads.push_back("SSAO_Noise");
	l_pass.m_Writes.push_back("SSAO_Result");

	auto l_addBinding = [&](const char* resource, GPUResourceType type, uint32_t set,
		uint32_t index, Accessibility bindAccess, Accessibility resAccess, TextureUsage usage)
	{
		BindingDesc b;
		b.m_Resource = resource;
		b.m_GPUResourceType = type;
		b.m_DescriptorSetIndex = set;
		b.m_DescriptorIndex = index;
		b.m_BindingAccessibility = bindAccess;
		b.m_ResourceAccessibility = resAccess;
		b.m_TextureUsage = usage;
		b.m_ShaderStage = ShaderStage::Compute;
		l_pass.m_Bindings.push_back(b);
	};

	l_addBinding("PerFrameCBuffer", GPUResourceType::Buffer, 0, 0, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::Invalid);
	l_addBinding("SSAO_Kernel", GPUResourceType::Buffer, 0, 1, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::Invalid);
	l_addBinding("OpaquePass_RT_0", GPUResourceType::Image, 1, 0, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::ColorAttachment);
	l_addBinding("OpaquePass_RT_1", GPUResourceType::Image, 1, 1, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::ColorAttachment);
	l_addBinding("SSAO_Noise", GPUResourceType::Image, 1, 2, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::ComputeOnly);
	l_addBinding("SSAONoisePass", GPUResourceType::Sampler, 2, 0, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::Invalid);
	l_addBinding("SSAONoisePass_RandomRot", GPUResourceType::Sampler, 2, 1, Accessibility::ReadOnly, Accessibility::ReadOnly, TextureUsage::Invalid);
	l_addBinding("SSAO_Result", GPUResourceType::Image, 3, 0, Accessibility::ReadWrite, Accessibility::ReadWrite, TextureUsage::ComputeOnly);

	TransitionDesc l_t0;
	l_t0.m_Resource = "SSAO_Noise";
	l_t0.m_From = Accessibility::WriteOnly;
	l_t0.m_To = Accessibility::ReadOnly;
	l_pass.m_Transitions.push_back(l_t0);

	TransitionDesc l_t1;
	l_t1.m_Resource = "SSAO_Result";
	l_t1.m_From = Accessibility::ReadOnly;
	l_t1.m_To = Accessibility::WriteOnly;
	l_pass.m_Transitions.push_back(l_t1);

	l_pass.m_Dispatch = { 1, 1, 1, DispatchMode::ScreenTile, 8 };
	l_pass.m_OneShot = false;
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

void TestSSAONodeRoundTrip()
{
	TestRunner::StartTest("RenderGraph: SSAO node (sampler bindings + 2 transitions + deferred screen Result) round-trips");

	RenderGraphDesc l_original = MakeSSAOGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = l_loaded.m_Resources.size() == 2 && l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[0];
		passed = r.m_Name == "SSAO_Result" &&
			r.m_Type == RenderGraphResourceType::Texture &&
			r.m_SizeExpr == "screen" &&
			r.m_Imported == false &&
			r.m_TextureDesc.Sampler == TextureSampler::Sampler2D &&
			r.m_TextureDesc.Usage == TextureUsage::ComputeOnly &&
			r.m_TextureDesc.PixelDataType == TexturePixelDataType::Float16 &&
			r.m_TextureDesc.PixelDataFormat == TexturePixelDataFormat::RGBA;
	}

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[1];
		passed = r.m_Name == "SSAO_Kernel" &&
			r.m_Type == RenderGraphResourceType::Buffer &&
			r.m_Imported == true;
	}

	if (passed)
	{
		const auto& p = l_loaded.m_Passes[0];
		passed = p.m_Name == "SSAONoisePass" &&
			p.m_Dispatch.m_Mode == DispatchMode::ScreenTile && p.m_Dispatch.m_TileSize == 8 &&
			p.m_Queue == GPUEngineType::Compute &&
			std::string(p.m_ShaderFilePaths.m_CSPath.c_str()) == "SSAONoisePass.comp" &&
			p.m_Writes.size() == 1 && p.m_Writes[0] == "SSAO_Result" &&
			p.m_Bindings.size() == 8 &&
			p.m_Transitions.size() == 2;
	}

	if (passed)
	{
		// Bindings round-trip by array position (== DX12 root parameter), the
		// order the kernel binds them; assert resource/type/set/index per slot.
		const auto& b = l_loaded.m_Passes[0].m_Bindings;
		passed =
			b[0].m_Resource == "PerFrameCBuffer" && b[0].m_GPUResourceType == GPUResourceType::Buffer && b[0].m_DescriptorSetIndex == 0 && b[0].m_DescriptorIndex == 0 &&
			b[1].m_Resource == "SSAO_Kernel" && b[1].m_GPUResourceType == GPUResourceType::Buffer && b[1].m_DescriptorSetIndex == 0 && b[1].m_DescriptorIndex == 1 &&
			b[2].m_Resource == "OpaquePass_RT_0" && b[2].m_GPUResourceType == GPUResourceType::Image && b[2].m_DescriptorSetIndex == 1 && b[2].m_DescriptorIndex == 0 &&
			b[3].m_Resource == "OpaquePass_RT_1" && b[3].m_GPUResourceType == GPUResourceType::Image && b[3].m_DescriptorSetIndex == 1 && b[3].m_DescriptorIndex == 1 &&
			b[4].m_Resource == "SSAO_Noise" && b[4].m_GPUResourceType == GPUResourceType::Image && b[4].m_DescriptorSetIndex == 1 && b[4].m_DescriptorIndex == 2;
	}

	if (passed)
	{
		const auto& b = l_loaded.m_Passes[0].m_Bindings;
		passed =
			b[5].m_Resource == "SSAONoisePass" && b[5].m_GPUResourceType == GPUResourceType::Sampler && b[5].m_DescriptorSetIndex == 2 && b[5].m_DescriptorIndex == 0 &&
			b[6].m_Resource == "SSAONoisePass_RandomRot" && b[6].m_GPUResourceType == GPUResourceType::Sampler && b[6].m_DescriptorSetIndex == 2 && b[6].m_DescriptorIndex == 1 &&
			b[7].m_Resource == "SSAO_Result" && b[7].m_GPUResourceType == GPUResourceType::Image && b[7].m_DescriptorSetIndex == 3 && b[7].m_DescriptorIndex == 0 &&
			b[7].m_BindingAccessibility == Accessibility::ReadWrite && b[7].m_ResourceAccessibility == Accessibility::ReadWrite &&
			b[7].m_TextureUsage == TextureUsage::ComputeOnly && b[2].m_TextureUsage == TextureUsage::ColorAttachment;
	}

	if (passed)
	{
		const auto& t = l_loaded.m_Passes[0].m_Transitions;
		passed =
			t[0].m_Resource == "SSAO_Noise" &&
			t[0].m_From == Accessibility::WriteOnly && t[0].m_To == Accessibility::ReadOnly &&
			t[1].m_Resource == "SSAO_Result" &&
			t[1].m_From == Accessibility::ReadOnly && t[1].m_To == Accessibility::WriteOnly;
	}

	TestRunner::EndTest(passed);
}
