#pragma once
enum class Accessibility
{
	Immutable = 1,
	ReadOnly = 2,
	WriteOnly = 4,
	ReadWrite = ReadOnly | WriteOnly
};

using Index = unsigned int;

enum class MeshUsageType { Static, Dynamic, Skeletal };
enum class MeshShapeType { Line, Quad, Cube, Sphere, Terrain, Custom };
enum class MeshPrimitiveTopology { Point, Line, Triangle, TriangleStrip };

enum class TextureSamplerType { Sampler1D, Sampler2D, Sampler3D, Sampler1DArray, Sampler2DArray, SamplerCubemap };

enum class TextureUsageType { Invisible, Normal, Albedo, Metallic, Roughness, AmbientOcclusion, ColorAttachment, DepthAttachment, DepthStencilAttachment, RawImage };

enum class TexturePixelDataFormat { R, RG, RGB, RGBA, BGRA, Depth, DepthStencil };
enum class TexturePixelDataType { UBYTE, SBYTE, USHORT, SSHORT, UINT8, SINT8, UINT16, SINT16, UINT32, SINT32, FLOAT16, FLOAT32, DOUBLE };
enum class TextureWrapMethod { Edge, Repeat, Border };
enum class TextureFilterMethod { Nearest, Linear, Mip };

struct TextureDataDesc
{
	Accessibility CPUAccessibility = Accessibility::Immutable;
	Accessibility GPUAccessibility = Accessibility::ReadWrite;
	TextureSamplerType SamplerType;
	TextureUsageType UsageType;
	TexturePixelDataFormat PixelDataFormat;
	TexturePixelDataType PixelDataType;
	TextureFilterMethod MinFilterMethod;
	TextureFilterMethod MagFilterMethod;
	TextureWrapMethod WrapMethod;
	unsigned int Width = 0;
	unsigned int Height = 0;
	unsigned int DepthOrArraySize = 0;
	float BorderColor[4];
};

enum class ComparisionFunction
{
	Never = 1,
	Less = 2,
	Equal = 3,
	LessEqual = 4,
	Greater = 5,
	NotEqual = 6,
	GreaterEqual = 7,
	Always = 8
};

enum class StencilOperation
{
	Keep = 1,
	Zero = 2,
	Replace = 3,
	IncreaseSat = 4,
	DecreaseSat = 5,
	Invert = 6,
	Increase = 7,
	Decrease = 8
};

enum class BlendFactor
{
	Zero = 1,
	One = 2,
	SrcColor = 3,
	OneMinusSrcColor = 4,
	SrcAlpha = 5,
	OneMinusSrcAlpha = 6,
	DestColor = 7,
	OneMinusDestColor = 8,
	DestAlpha = 9,
	OneMinusDestAlpha = 10,
	Src1Color = 11,
	OneMinusSrc1Color = 12,
	Src1Alpha = 13,
	OneMinusSrc1Alpha = 14
};

enum class BlendOperation
{
	Add = 1,
	Substruct = 2
};

enum class PrimitiveTopology
{
	Point = 1,
	Line = 2,
	TriangleList = 3,
	TriangleStrip = 4,
	Patch = 5
};

enum class RasterizerFillMode
{
	Point = 1,
	Wireframe = 2,
	Solid = 3
};

enum class RasterizerFaceWinding
{
	CCW = 1,
	CW = 2
};

enum class RasterizerCullMode
{
	Back = 1,
	Front = 2
};

struct DepthStencilDesc
{
	bool m_UseDepthBuffer = false;
	bool m_AllowDepthWrite = false;
	ComparisionFunction m_DepthComparisionFunction = ComparisionFunction::Never;
	bool m_AllowDepthClamp = true;

	bool m_UseStencilBuffer = false;
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

enum class RenderPassUsageType
{
	Graphics = 1,
	Compute = 2
};

struct RenderPassDesc
{
	bool m_UseMultiFrames = false;
	RenderPassUsageType m_RenderPassUsageType = RenderPassUsageType::Graphics;
	size_t m_RenderTargetCount = 0;
	TextureDataDesc m_RenderTargetDesc = {};
	GraphicsPipelineDesc m_GraphicsPipelineDesc = {};
};

class IPipelineStateObject {};

enum class ResourceBinderType {
	Sampler,
	Image,
	Buffer,
};

class IResourceBinder
{
public:
	ResourceBinderType m_ResourceBinderType = ResourceBinderType::Sampler;
	Accessibility m_GPUAccessibility = Accessibility::ReadOnly;
	size_t m_ElementSize = 0;
};

struct ResourceBinderLayoutDesc
{
	ResourceBinderType m_ResourceBinderType = ResourceBinderType::Sampler;
	Accessibility m_BinderAccessibility = Accessibility::ReadOnly;
	Accessibility m_ResourceAccessibility = Accessibility::ReadOnly;
	// The global shader root register slot index, useless for OpenGL and DirectX 11
	size_t m_GlobalSlot = 0;
	// The local type related register slot index, like "binding = 5" in GLSL or  "t(4)"/"c(3)" in HLSL
	size_t m_LocalSlot = 0;
	size_t m_ResourceCount = 1;
	bool m_IsRanged = false;
};

class ICommandList {};
class ICommandQueue {};
class ISemaphore {};
class IFence {};
