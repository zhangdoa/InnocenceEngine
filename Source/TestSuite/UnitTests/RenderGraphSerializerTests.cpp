#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"
#include "../../Engine/RenderGraph/RenderGraphEnumStrings.h"

using namespace Inno;

static RenderGraphDesc MakeBRDFLUTGraph()
{
	RenderGraphDesc l_graph;
	l_graph.m_Name = "ExampleRenderGraph";

	ResourceDesc l_res;
	l_res.m_Name = "BRDF LUT";
	l_res.m_Type = RenderGraphResourceType::Texture;
	l_res.m_TextureDesc.Width = 512;
	l_res.m_TextureDesc.Height = 512;
	l_res.m_TextureDesc.DepthOrArraySize = 1;
	l_res.m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	l_res.m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	l_res.m_TextureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_res.m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	l_res.m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_graph.m_Resources.push_back(l_res);

	PassNodeDesc l_pass;
	l_pass.m_Name = "BRDFLUTPass";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_ShaderFilePaths.m_CSPath = "BRDFLUTPass.comp";
	l_pass.m_Writes.push_back("BRDF LUT");
	BindingDesc l_binding;
	l_binding.m_Resource = "BRDF LUT";
	l_binding.m_GPUResourceType = GPUResourceType::Image;
	l_binding.m_BindingAccessibility = Accessibility::ReadWrite;
	l_binding.m_ResourceAccessibility = Accessibility::ReadWrite;
	l_binding.m_TextureUsage = TextureUsage::ComputeOnly;
	l_binding.m_ShaderStage = ShaderStage::Compute;
	l_pass.m_Bindings.push_back(l_binding);
	l_pass.m_Dispatch = { 32, 32, 1 };
	l_pass.m_OneShot = true;
	l_graph.m_Passes.push_back(l_pass);

	return l_graph;
}

static void TestEnumRoundTrip()
{
	TestRunner::StartTest("RenderGraph: enum string round-trip preserves values");

	namespace ES = RenderGraphEnumStrings;
	bool passed =
		ES::TextureSamplerFromString(ES::ToString(TextureSampler::Sampler2D)) == TextureSampler::Sampler2D &&
		ES::TextureUsageFromString(ES::ToString(TextureUsage::ComputeOnly)) == TextureUsage::ComputeOnly &&
		ES::TexturePixelDataTypeFromString(ES::ToString(TexturePixelDataType::Float16)) == TexturePixelDataType::Float16 &&
		ES::TexturePixelDataFormatFromString(ES::ToString(TexturePixelDataFormat::RGBA)) == TexturePixelDataFormat::RGBA &&
		ES::ShaderStageFromString(ES::ToString(ShaderStage::Compute)) == ShaderStage::Compute &&
		ES::GPUResourceTypeFromString(ES::ToString(GPUResourceType::Image)) == GPUResourceType::Image &&
		ES::GPUEngineTypeFromString(ES::ToString(GPUEngineType::Compute)) == GPUEngineType::Compute &&
		ES::AccessibilityFromString(ES::ToString(Accessibility::ReadWrite)) == Accessibility::ReadWrite;

	TestRunner::EndTest(passed);
}

static void TestGraphRoundTrip()
{
	TestRunner::StartTest("RenderGraph: BRDFLUT graph survives to_json/from_json round-trip");

	RenderGraphDesc l_original = MakeBRDFLUTGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed =
		l_loaded.m_Name == "ExampleRenderGraph" &&
		l_loaded.m_Resources.size() == 1 &&
		l_loaded.m_Passes.size() == 1;

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[0];
		passed = r.m_Name == "BRDF LUT" &&
			r.m_TextureDesc.Width == 512 && r.m_TextureDesc.Height == 512 &&
			r.m_TextureDesc.DepthOrArraySize == 1 &&
			r.m_TextureDesc.Sampler == TextureSampler::Sampler2D &&
			r.m_TextureDesc.Usage == TextureUsage::ComputeOnly &&
			r.m_TextureDesc.GPUAccessibility == Accessibility::ReadWrite &&
			r.m_TextureDesc.PixelDataType == TexturePixelDataType::Float16 &&
			r.m_TextureDesc.PixelDataFormat == TexturePixelDataFormat::RGBA;
	}

	if (passed)
	{
		const auto& p = l_loaded.m_Passes[0];
		passed = p.m_Name == "BRDFLUTPass" &&
			p.m_Queue == GPUEngineType::Compute &&
			p.m_Kernel == "Default" &&
			std::string(p.m_ShaderFilePaths.m_CSPath.c_str()) == "BRDFLUTPass.comp" &&
			p.m_Writes.size() == 1 && p.m_Writes[0] == "BRDF LUT" &&
			p.m_Bindings.size() == 1 &&
			p.m_Dispatch.m_X == 32 && p.m_Dispatch.m_Y == 32 && p.m_Dispatch.m_Z == 1 &&
			p.m_OneShot == true;
	}

	if (passed)
	{
		const auto& b = l_loaded.m_Passes[0].m_Bindings[0];
		passed = b.m_Resource == "BRDF LUT" &&
			b.m_GPUResourceType == GPUResourceType::Image &&
			b.m_DescriptorSetIndex == 0 && b.m_DescriptorIndex == 0 &&
			b.m_BindingAccessibility == Accessibility::ReadWrite &&
			b.m_ResourceAccessibility == Accessibility::ReadWrite &&
			b.m_TextureUsage == TextureUsage::ComputeOnly &&
			b.m_ShaderStage == ShaderStage::Compute;
	}

	TestRunner::EndTest(passed);
}

static RenderGraphDesc MakeBufferGraph()
{
	// Mirrors LuminanceAveragePass: a graph-owned output buffer plus an
	// imported input buffer produced by a still-imperative pass.
	RenderGraphDesc l_graph;
	l_graph.m_Name = "BufferGraph";

	ResourceDesc l_owned;
	l_owned.m_Name = "LuminanceAverageGPUBuffer";
	l_owned.m_Type = RenderGraphResourceType::Buffer;
	l_owned.m_BufferDesc.m_ElementCount = 16;
	l_owned.m_BufferDesc.m_ElementSize = sizeof(float);
	l_owned.m_BufferDesc.m_Usage = GPUBufferUsage::Generic;
	l_owned.m_BufferDesc.m_CPUAccessibility = Accessibility::Immutable;
	l_owned.m_BufferDesc.m_GPUAccessibility = Accessibility::ReadWrite;
	l_graph.m_Resources.push_back(l_owned);

	ResourceDesc l_imported;
	l_imported.m_Name = "PerFrameCBuffer";
	l_imported.m_Type = RenderGraphResourceType::Buffer;
	l_imported.m_Imported = true;
	l_graph.m_Resources.push_back(l_imported);

	return l_graph;
}

static void TestBufferRoundTrip()
{
	TestRunner::StartTest("RenderGraph: buffer + imported resource survives round-trip");

	RenderGraphDesc l_original = MakeBufferGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = l_loaded.m_Resources.size() == 2;

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[0];
		passed = r.m_Name == "LuminanceAverageGPUBuffer" &&
			r.m_Type == RenderGraphResourceType::Buffer &&
			r.m_Imported == false &&
			r.m_BufferDesc.m_ElementCount == 16 &&
			r.m_BufferDesc.m_ElementSize == sizeof(float) &&
			r.m_BufferDesc.m_Usage == GPUBufferUsage::Generic &&
			r.m_BufferDesc.m_CPUAccessibility == Accessibility::Immutable &&
			r.m_BufferDesc.m_GPUAccessibility == Accessibility::ReadWrite;
	}

	if (passed)
	{
		const auto& r = l_loaded.m_Resources[1];
		passed = r.m_Name == "PerFrameCBuffer" &&
			r.m_Type == RenderGraphResourceType::Buffer &&
			r.m_Imported == true;
	}

	TestRunner::EndTest(passed);
}

void RunRenderGraphSerializerUnitTests()
{
	TestRunner::StartTestSuite("RenderGraphSerializer");
	TestEnumRoundTrip();
	TestGraphRoundTrip();
	TestBufferRoundTrip();
	TestRunner::EndTestSuite();
}
