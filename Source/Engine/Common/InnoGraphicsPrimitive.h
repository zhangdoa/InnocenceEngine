#pragma once

namespace InnoType
{
	enum class Accessibility
	{
		Immutable,
		ReadOnly,
		WriteOnly,
		ReadWrite
	};

	using Index = uint32_t;

	enum class MeshUsage { Invalid, Static, Dynamic, Skeletal };
	enum class MeshSource { Invalid, Procedural, Customized };
	enum class MeshPrimitiveTopology { Point, Line, Triangle, TriangleStrip };
	enum class ProceduralMeshShape { Invalid, Triangle, Square, Pentagon, Hexagon, Tetrahedron, Cube, Octahedron, Dodecahedron, Icosahedron, Sphere };

	enum class TextureSampler { Invalid, Sampler1D, Sampler2D, Sampler3D, Sampler1DArray, Sampler2DArray, SamplerCubemap };
	enum class TextureUsage { Invalid, Sample, ColorAttachment, DepthAttachment, DepthStencilAttachment, RawImage };
	enum class TexturePixelDataFormat { Invalid, R, RG, RGB, RGBA, BGRA, Depth, DepthStencil };
	enum class TexturePixelDataType { Invalid, UByte, SByte, UShort, SShort, UInt8, SInt8, UInt16, SInt16, UInt32, SInt32, Float16, Float32, Double };
	enum class TextureWrapMethod { Invalid, Edge, Repeat, Border };
	enum class TextureFilterMethod { Invalid, Nearest, Linear };

	struct TextureDesc
	{
		Accessibility CPUAccessibility = Accessibility::Immutable;
		Accessibility GPUAccessibility = Accessibility::ReadWrite;
		TextureSampler Sampler;
		TextureUsage Usage;
		bool IsSRGB = false;
		TexturePixelDataFormat PixelDataFormat;
		TexturePixelDataType PixelDataType;
		bool UseMipMap = false;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t DepthOrArraySize = 0;
		float BorderColor[4];
	};

	enum class ComparisionFunction
	{
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always
	};

	enum class StencilOperation
	{
		Keep,
		Zero,
		Replace,
		IncreaseSat,
		DecreaseSat,
		Invert,
		Increase,
		Decrease
	};

	enum class BlendFactor
	{
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		SrcAlpha,
		OneMinusSrcAlpha,
		DestColor,
		OneMinusDestColor,
		DestAlpha,
		OneMinusDestAlpha,
		Src1Color,
		OneMinusSrc1Color,
		Src1Alpha,
		OneMinusSrc1Alpha
	};

	enum class BlendOperation
	{
		Add,
		Substruct
	};

	enum class PrimitiveTopology
	{
		Point,
		Line,
		TriangleList,
		TriangleStrip,
		Patch
	};

	enum class RasterizerFillMode
	{
		Point,
		Wireframe,
		Solid
	};

	enum class RasterizerFaceWinding
	{
		CCW,
		CW
	};

	enum class RasterizerCullMode
	{
		Back,
		Front
	};

	struct DepthStencilDesc
	{
		bool m_DepthEnable = false;
		bool m_AllowDepthWrite = false;
		ComparisionFunction m_DepthComparisionFunction = ComparisionFunction::Never;
		bool m_AllowDepthClamp = false;

		bool m_StencilEnable = false;
		bool m_AllowStencilWrite = false;
		uint8_t m_StencilReference = 0x00;
		uint8_t m_StencilWriteMask = 0xFF;
		StencilOperation m_FrontFaceStencilFailOperation = StencilOperation::Keep;
		StencilOperation m_FrontFaceStencilPassDepthFailOperation = StencilOperation::Keep;
		StencilOperation m_FrontFaceStencilPassOperation = StencilOperation::Keep;
		ComparisionFunction m_FrontFaceStencilComparisionFunction = ComparisionFunction::Never;
		StencilOperation m_BackFaceStencilFailOperation = StencilOperation::Keep;
		StencilOperation m_BackFaceStencilPassDepthFailOperation = StencilOperation::Keep;
		StencilOperation m_BackFaceStencilPassOperation = StencilOperation::Keep;
		ComparisionFunction m_BackFaceStencilComparisionFunction = ComparisionFunction::Never;
	};

	struct BlendDesc
	{
		bool m_UseBlend = false;
		BlendFactor m_SourceRGBFactor = BlendFactor::SrcColor;
		BlendFactor m_SourceAlphaFactor = BlendFactor::SrcAlpha;
		BlendFactor m_DestinationRGBFactor = BlendFactor::DestColor;
		BlendFactor m_DestinationAlphaFactor = BlendFactor::DestAlpha;
		BlendOperation m_BlendOperation = BlendOperation::Add;
	};

	struct RasterizerDesc
	{
		PrimitiveTopology m_PrimitiveTopology = PrimitiveTopology::TriangleList;
		RasterizerFillMode m_RasterizerFillMode = RasterizerFillMode::Solid;
		RasterizerFaceWinding m_RasterizerFaceWinding = RasterizerFaceWinding::CCW;
		bool m_UseCulling = false;
		RasterizerCullMode m_RasterizerCullMode = RasterizerCullMode::Back;
		bool m_AllowMultisample = false;
	};

	struct ViewportDesc
	{
		float m_OriginX = 0.0f;
		float m_OriginY = 0.0f;
		float m_Width = 0.0f;
		float m_Height = 0.0f;
		float m_MinDepth = 0.0f;
		float m_MaxDepth = 1.0f;
	};

	struct GraphicsPipelineDesc
	{
		DepthStencilDesc m_DepthStencilDesc = {};
		BlendDesc m_BlendDesc = {};
		RasterizerDesc m_RasterizerDesc = {};
		ViewportDesc m_ViewportDesc = {};
		float CleanColor[4] = { 0.0f , 0.0f, 0.0f, 0.0f };
	};

	enum class RenderPassUsage
	{
		Invalid,
		Graphics,
		Compute
	};

	struct RenderPassDesc
	{
		bool m_UseMultiFrames = false;
		bool m_IsOffScreen = false;
		RenderPassUsage m_RenderPassUsage = RenderPassUsage::Graphics;
		size_t m_RenderTargetCount = 0;
		bool m_UseColorBuffer = true;
		bool m_UseDepthBuffer = false;
		bool m_UseStencilBuffer = false;
		TextureDesc m_RenderTargetDesc = {};
		GraphicsPipelineDesc m_GraphicsPipelineDesc = {};
	};

	class IPipelineStateObject {};

	enum class ResourceBinderType {
		Sampler,
		Image,
		Buffer
	};

	class IResourceBinder
	{
	public:
		ResourceBinderType m_ResourceBinderType = ResourceBinderType::Sampler;
		Accessibility m_GPUAccessibility = Accessibility::ReadOnly;
		size_t m_ElementCount = 0;
		size_t m_ElementSize = 0;
		size_t m_TotalSize = 0;
	};

	struct ResourceBinderLayoutDesc
	{
		ResourceBinderType m_ResourceBinderType = ResourceBinderType::Sampler;
		Accessibility m_BinderAccessibility = Accessibility::ReadOnly;
		Accessibility m_ResourceAccessibility = Accessibility::ReadOnly;
		size_t m_DescriptorSetIndex = 0;
		size_t m_DescriptorIndex = 0;
		size_t m_ResourceCount = 1;
		bool m_IndirectBinding = false;
	};
}

using namespace InnoType;

class ICommandList {};
class ICommandQueue {};
class ISemaphore {};
class IFence {};
