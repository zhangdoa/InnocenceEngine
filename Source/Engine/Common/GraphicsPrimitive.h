#pragma once
#include "STL14.h"
#include "Enum.h"

namespace Inno
{
	class GPUResourceComponent;
	namespace Type
	{
		class Accessibility
		{
		public:
			explicit Accessibility(bool read, bool write)
				: m_Read(read)
				, m_Write(write)
			{
			}

			bool operator==(const Accessibility& rhs) const
			{
				return m_Read == rhs.m_Read && m_Write == rhs.m_Write;
			}

			bool operator!=(const Accessibility& rhs) const
			{
				return !(*this == rhs);
			}
			
			static Accessibility Immutable;
			static Accessibility ReadOnly;
			static Accessibility WriteOnly;
			static Accessibility ReadWrite;

			bool CanRead() const { return m_Read; }
			bool CanWrite() const { return m_Write; }

		private:
			bool m_Read = false;
			bool m_Write = false;
		};

		using Index = uint32_t;

		enum class MeshUsage { Invalid, Static, Dynamic, Skeletal };
		enum class MeshShape { Invalid, Customized, Triangle, Square, Pentagon, Hexagon, Tetrahedron, Cube, Octahedron, Dodecahedron, Icosahedron, Sphere };

		enum class TextureSampler { Invalid, Sampler1D, Sampler2D, Sampler3D, Sampler1DArray, Sampler2DArray, SamplerCubemap };
		enum class TextureUsage { Invalid, Sample, ColorAttachment, DepthAttachment, DepthStencilAttachment };
		enum class TexturePixelDataFormat { Invalid, R, RG, RGB, RGBA, BGRA, Depth, DepthStencil };
		enum class TexturePixelDataType { Invalid, UByte, SByte, UShort, SShort, UInt8, SInt8, UInt16, SInt16, UInt32, SInt32, Float16, Float32, Double };
		enum class TextureWrapMethod { Invalid, Edge, Repeat, Border };
		enum class TextureFilterMethod { Invalid, Nearest, Linear };

		struct TextureDesc
		{
			Accessibility CPUAccessibility = Accessibility(false, false);
			Accessibility GPUAccessibility = Accessibility(true, true);

			TextureSampler Sampler = TextureSampler::Invalid;
			TextureUsage Usage = TextureUsage::Invalid;
			bool IsSRGB = false;
			TexturePixelDataFormat PixelDataFormat = TexturePixelDataFormat::Invalid;
			TexturePixelDataType PixelDataType = TexturePixelDataType::Invalid;
			bool UseMipMap = false;
			uint32_t Width = 0;
			uint32_t Height = 0;
			uint32_t DepthOrArraySize = 0;
			float BorderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
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
		};

		enum class GPUEngineType
		{
			Invalid,
			Graphics,
			Compute
		};

		enum class ShaderStage
		{
			Invalid = 0x00000000,
			Vertex = 0x00000001,
			Hull = 0x00000002,
			Domain = 0x00000004,
			Geometry = 0x00000008,
			Pixel = 0x00000010,
			Compute = 0x00000020
		};

		INNO_ENUM_OPERATORS(ShaderStage);

		struct RenderPassDesc
		{
			bool m_UseMultiFrames = false;
			bool m_Resizable = true;
			GPUEngineType m_GPUEngineType = GPUEngineType::Graphics;
			size_t m_RenderTargetCount = 0;
			bool m_UseDepthBuffer = false;
			bool m_UseStencilBuffer = false;
			bool m_UseOutputMerger = true;
			TextureDesc m_RenderTargetDesc = {};
			GraphicsPipelineDesc m_GraphicsPipelineDesc = {};
			std::function<bool()> m_RenderTargetsReservationFunc;
			std::function<bool()> m_RenderTargetsCreationFunc;
			std::function<bool()> m_DepthStencilRenderTargetsReservationFunc;
			std::function<bool()> m_DepthStencilRenderTargetsCreationFunc;
		};

		struct IPipelineStateObject {};

		enum class GPUResourceType 
		{
			Invalid,
			Sampler,
			Image,
			Buffer,
			Material
		};

		enum class GPUMemoryType
		{
			Invalid,
			Default,
			Upload,
			Readback
		};

		struct IGPUMemory {};

		const uint32_t MaxDescriptorCount = 65536;
		struct ResourceBindingLayoutDesc
		{
			GPUResourceType m_GPUResourceType = GPUResourceType::Sampler;
			GPUResourceComponent* m_GPUResource = nullptr;
			ShaderStage m_ShaderStage = ShaderStage::Invalid;
			Accessibility m_BindingAccessibility = Accessibility::ReadOnly;
			Accessibility m_ResourceAccessibility = Accessibility::ReadOnly;
			uint32_t m_DescriptorSetIndex = 0;
			uint32_t m_DescriptorIndex = 0;
			uint32_t m_SubresourceCount = 1;
			bool m_IndirectBinding = false;
			bool m_IsRootConstant = false;
		};
	}

	struct ICommandList {};

	struct ISemaphore {};
}

using namespace Inno::Type;