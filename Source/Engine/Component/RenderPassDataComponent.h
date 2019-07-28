#pragma once
#include "../Common/InnoType.h"
#include "../Component/TextureDataComponent.h"

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

struct SamplerDesc
{
	TextureFilterMethod m_FilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
	TextureWrapMethod m_WrapMethodU = TextureWrapMethod::CLAMP_TO_EDGE;
	TextureWrapMethod m_WrapMethodV = TextureWrapMethod::CLAMP_TO_EDGE;
	TextureWrapMethod m_WrapMethodW = TextureWrapMethod::CLAMP_TO_EDGE;
	float m_BoardColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float m_MinLOD = 0.0f;
	float m_MaxLOD = 3.402823466e+38f;
	unsigned int m_MaxAnisotropy = 1;
};

struct GraphicsPipelineDesc
{
	DepthStencilDesc m_DepthStencilDesc = {};
	BlendDesc m_BlendDesc = {};
	RasterizerDesc m_RasterizerDesc = {};
	ViewportDesc m_ViewportDesc = {};
	SamplerDesc m_SamplerDesc = {};
};

struct RenderPassDesc
{
	bool m_UseMultiFrames = false;
	size_t m_RenderTargetCount = 0;
	TextureDataDesc m_RenderTargetDesc = {};
	GraphicsPipelineDesc m_GraphicsPipelineDesc = {};
};

class IPipelineStateObject
{
};

class ICommandList
{
};

class ICommandQueue
{
};

class ISemaphore
{
};

class IFence
{
};

class RenderPassDataComponent : public InnoComponent
{
public:
	RenderPassDesc m_RenderPassDesc = {};

	std::vector<TextureDataComponent*> m_RenderTargets;
	TextureDataComponent* m_DepthStencilRenderTarget;

	IPipelineStateObject* m_PipelineStateObject;

	ICommandQueue* m_CommandQueue;
	std::vector<ICommandList*> m_CommandLists;

	size_t m_CurrentFrame = 0;

	std::vector<ISemaphore*> m_waitSemaphores;
	std::vector<ISemaphore*> m_singalSemaphores;
	std::vector<IFence*> m_Fences;
};