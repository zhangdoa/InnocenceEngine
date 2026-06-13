#include "RenderGraphEnumStrings.h"
#include <utility>
#include <vector>

using namespace Inno;

namespace
{
	// One row per enumerator. ToString does a forward lookup, FromString the
	// inverse; both share the same table so the two directions cannot drift.
	template <typename EnumT>
	using Row = std::pair<EnumT, const char*>;

	template <typename EnumT>
	std::string ToStringImpl(EnumT v, const std::vector<Row<EnumT>>& table, const char* fallback)
	{
		for (const auto& row : table)
			if (row.first == v)
				return row.second;
		return fallback;
	}

	template <typename EnumT>
	EnumT FromStringImpl(const std::string& s, const std::vector<Row<EnumT>>& table, EnumT fallback)
	{
		for (const auto& row : table)
			if (s == row.second)
				return row.first;
		return fallback;
	}

	const std::vector<Row<GPUEngineType>> g_GPUEngineType = {
		{ GPUEngineType::Invalid, "Invalid" }, { GPUEngineType::Graphics, "Graphics" },
		{ GPUEngineType::Compute, "Compute" }, { GPUEngineType::Copy, "Copy" } };

	const std::vector<Row<TextureSampler>> g_TextureSampler = {
		{ TextureSampler::Invalid, "Invalid" }, { TextureSampler::Sampler1D, "Sampler1D" },
		{ TextureSampler::Sampler2D, "Sampler2D" }, { TextureSampler::Sampler3D, "Sampler3D" },
		{ TextureSampler::Sampler1DArray, "Sampler1DArray" }, { TextureSampler::Sampler2DArray, "Sampler2DArray" },
		{ TextureSampler::SamplerCubemap, "SamplerCubemap" } };

	const std::vector<Row<TextureUsage>> g_TextureUsage = {
		{ TextureUsage::Invalid, "Invalid" }, { TextureUsage::Sample, "Sample" },
		{ TextureUsage::ColorAttachment, "ColorAttachment" }, { TextureUsage::DepthAttachment, "DepthAttachment" },
		{ TextureUsage::DepthStencilAttachment, "DepthStencilAttachment" }, { TextureUsage::ComputeOnly, "ComputeOnly" } };

	const std::vector<Row<TexturePixelDataFormat>> g_PixelDataFormat = {
		{ TexturePixelDataFormat::Invalid, "Invalid" }, { TexturePixelDataFormat::R, "R" },
		{ TexturePixelDataFormat::RG, "RG" }, { TexturePixelDataFormat::RGB, "RGB" },
		{ TexturePixelDataFormat::RGBA, "RGBA" }, { TexturePixelDataFormat::BGRA, "BGRA" },
		{ TexturePixelDataFormat::Depth, "Depth" }, { TexturePixelDataFormat::DepthStencil, "DepthStencil" },
		{ TexturePixelDataFormat::BC1, "BC1" }, { TexturePixelDataFormat::BC3, "BC3" },
		{ TexturePixelDataFormat::BC4, "BC4" }, { TexturePixelDataFormat::BC5, "BC5" },
		{ TexturePixelDataFormat::RGB10A2, "RGB10A2" } };

	const std::vector<Row<TexturePixelDataType>> g_PixelDataType = {
		{ TexturePixelDataType::Invalid, "Invalid" }, { TexturePixelDataType::UByte, "UByte" },
		{ TexturePixelDataType::SByte, "SByte" }, { TexturePixelDataType::UShort, "UShort" },
		{ TexturePixelDataType::SShort, "SShort" }, { TexturePixelDataType::UInt8, "UInt8" },
		{ TexturePixelDataType::SInt8, "SInt8" }, { TexturePixelDataType::UInt16, "UInt16" },
		{ TexturePixelDataType::SInt16, "SInt16" }, { TexturePixelDataType::UInt32, "UInt32" },
		{ TexturePixelDataType::SInt32, "SInt32" }, { TexturePixelDataType::Float16, "Float16" },
		{ TexturePixelDataType::Float32, "Float32" }, { TexturePixelDataType::Double, "Double" },
		{ TexturePixelDataType::Compressed, "Compressed" } };

	const std::vector<Row<GPUResourceType>> g_GPUResourceType = {
		{ GPUResourceType::Invalid, "Invalid" }, { GPUResourceType::Sampler, "Sampler" },
		{ GPUResourceType::Image, "Image" }, { GPUResourceType::Buffer, "Buffer" },
		{ GPUResourceType::Material, "Material" } };

	const std::vector<Row<ShaderStage>> g_ShaderStage = {
		{ ShaderStage::Invalid, "Invalid" }, { ShaderStage::Vertex, "Vertex" },
		{ ShaderStage::Hull, "Hull" }, { ShaderStage::Domain, "Domain" },
		{ ShaderStage::Geometry, "Geometry" }, { ShaderStage::Pixel, "Pixel" },
		{ ShaderStage::Compute, "Compute" }, { ShaderStage::RayGen, "RayGen" },
		{ ShaderStage::ClosestHit, "ClosestHit" }, { ShaderStage::AnyHit, "AnyHit" },
		{ ShaderStage::Miss, "Miss" } };

	const std::vector<Row<GPUBufferUsage>> g_GPUBufferUsage = {
		{ GPUBufferUsage::Generic, "Generic" }, { GPUBufferUsage::IndirectDraw, "IndirectDraw" },
		{ GPUBufferUsage::IndirectDispatch, "IndirectDispatch" }, { GPUBufferUsage::AtomicCounter, "AtomicCounter" },
		{ GPUBufferUsage::TLAS, "TLAS" }, { GPUBufferUsage::ScratchBuffer, "ScratchBuffer" },
		{ GPUBufferUsage::BindlessMeshVertex, "BindlessMeshVertex" }, { GPUBufferUsage::BindlessMeshIndex, "BindlessMeshIndex" } };

	const std::vector<Row<RenderGraphResourceType>> g_ResourceType = {
		{ RenderGraphResourceType::Texture, "Texture" }, { RenderGraphResourceType::Buffer, "Buffer" } };

	const std::vector<Row<RenderGraphResourceLifetime>> g_Lifetime = {
		{ RenderGraphResourceLifetime::Persistent, "Persistent" } };

	const std::vector<Row<DispatchMode>> g_DispatchMode = {
		{ DispatchMode::Static, "Static" }, { DispatchMode::ScreenTile, "ScreenTile" },
		{ DispatchMode::TiledTwoLevel, "TiledTwoLevel" }, { DispatchMode::DrawModelGroups, "DrawModelGroups" },
		{ DispatchMode::DispatchRays, "DispatchRays" }, { DispatchMode::TiledDispatch, "TiledDispatch" } };

	const std::vector<Row<ComparisionFunction>> g_ComparisionFunction = {
		{ ComparisionFunction::Never, "Never" }, { ComparisionFunction::Less, "Less" },
		{ ComparisionFunction::Equal, "Equal" }, { ComparisionFunction::LessEqual, "LessEqual" },
		{ ComparisionFunction::Greater, "Greater" }, { ComparisionFunction::NotEqual, "NotEqual" },
		{ ComparisionFunction::GreaterEqual, "GreaterEqual" }, { ComparisionFunction::Always, "Always" } };
}

namespace Inno
{
	namespace RenderGraphEnumStrings
	{
		std::string ToString(GPUEngineType v) { return ToStringImpl(v, g_GPUEngineType, "Invalid"); }
		GPUEngineType GPUEngineTypeFromString(const std::string& s) { return FromStringImpl(s, g_GPUEngineType, GPUEngineType::Invalid); }

		std::string ToString(TextureSampler v) { return ToStringImpl(v, g_TextureSampler, "Invalid"); }
		TextureSampler TextureSamplerFromString(const std::string& s) { return FromStringImpl(s, g_TextureSampler, TextureSampler::Invalid); }

		std::string ToString(TextureUsage v) { return ToStringImpl(v, g_TextureUsage, "Invalid"); }
		TextureUsage TextureUsageFromString(const std::string& s) { return FromStringImpl(s, g_TextureUsage, TextureUsage::Invalid); }

		std::string ToString(TexturePixelDataFormat v) { return ToStringImpl(v, g_PixelDataFormat, "Invalid"); }
		TexturePixelDataFormat TexturePixelDataFormatFromString(const std::string& s) { return FromStringImpl(s, g_PixelDataFormat, TexturePixelDataFormat::Invalid); }

		std::string ToString(TexturePixelDataType v) { return ToStringImpl(v, g_PixelDataType, "Invalid"); }
		TexturePixelDataType TexturePixelDataTypeFromString(const std::string& s) { return FromStringImpl(s, g_PixelDataType, TexturePixelDataType::Invalid); }

		std::string ToString(GPUResourceType v) { return ToStringImpl(v, g_GPUResourceType, "Invalid"); }
		GPUResourceType GPUResourceTypeFromString(const std::string& s) { return FromStringImpl(s, g_GPUResourceType, GPUResourceType::Invalid); }

		std::string ToString(ShaderStage v) { return ToStringImpl(v, g_ShaderStage, "Invalid"); }
		ShaderStage ShaderStageFromString(const std::string& s) { return FromStringImpl(s, g_ShaderStage, ShaderStage::Invalid); }

		std::string ToString(Accessibility v)
		{
			if (v == Accessibility::ReadWrite) return "ReadWrite";
			if (v == Accessibility::WriteOnly) return "WriteOnly";
			if (v == Accessibility::Immutable) return "Immutable";
			return "ReadOnly";
		}
		Accessibility AccessibilityFromString(const std::string& s)
		{
			if (s == "ReadWrite") return Accessibility::ReadWrite;
			if (s == "WriteOnly") return Accessibility::WriteOnly;
			if (s == "Immutable") return Accessibility::Immutable;
			return Accessibility::ReadOnly;
		}

		std::string ToString(GPUBufferUsage v) { return ToStringImpl(v, g_GPUBufferUsage, "Generic"); }
		GPUBufferUsage GPUBufferUsageFromString(const std::string& s) { return FromStringImpl(s, g_GPUBufferUsage, GPUBufferUsage::Generic); }

		std::string ToString(RenderGraphResourceType v) { return ToStringImpl(v, g_ResourceType, "Texture"); }
		RenderGraphResourceType ResourceTypeFromString(const std::string& s) { return FromStringImpl(s, g_ResourceType, RenderGraphResourceType::Texture); }

		std::string ToString(RenderGraphResourceLifetime v) { return ToStringImpl(v, g_Lifetime, "Persistent"); }
		RenderGraphResourceLifetime LifetimeFromString(const std::string& s) { return FromStringImpl(s, g_Lifetime, RenderGraphResourceLifetime::Persistent); }

		std::string ToString(DispatchMode v) { return ToStringImpl(v, g_DispatchMode, "Static"); }
		DispatchMode DispatchModeFromString(const std::string& s) { return FromStringImpl(s, g_DispatchMode, DispatchMode::Static); }

		std::string ToString(ComparisionFunction v) { return ToStringImpl(v, g_ComparisionFunction, "Never"); }
		ComparisionFunction ComparisionFunctionFromString(const std::string& s) { return FromStringImpl(s, g_ComparisionFunction, ComparisionFunction::Never); }
	}
}
