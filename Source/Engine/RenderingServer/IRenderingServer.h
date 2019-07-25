#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"

#include "../Component/MeshDataComponent.h"
#include "../Component/TextureDataComponent.h"
#include "../Component/MaterialDataComponent.h"

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

enum class PrimitiveTopologyType
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
}

enum class RasterizerCullMode
{
	Back = 1,
	Front = 2
}

struct DepthStencilDesc
{
	bool m_UseDepthBuffer = false;
	bool m_UseStencilBuffer = false;

	bool m_AllowDepthWrite = false;
	ComparisionFunction m_DepthComparisionFunction = ComparisionFunction::Never;

	uint8_t m_StencilReadMask = 0xFF;
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
	BlendFactor m_BlendFactor = BlendFactor::Zero;
	BlendOperation m_BlendOperation = BlendOperation::Add;
};

struct RasterizerDesc
{
	PrimitiveTopologyType m_PrimitiveTopologyType = PrimitiveTopologyType::TriangleStrip;
	RasterizerFillMode m_RasterizerFillMode = RasterizerFillMode::Solid;
	RasterizerFaceWinding m_RasterizerFaceWinding = RasterizerFaceWinding::CCW;
	RasterizerCullMode m_RasterizerCullMode = RasterizerCullMode::Back;
	bool m_AllowDepthClamp = true;
	bool m_AllowMultisample = false;
};

struct ViewportDesc
{
	float m_TopLeftX = 0.0f;
	float m_TopLeftY = 0.0f;
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

class RenderPassComponent : public InnoComponent
{
public:
	RenderPassComponent() {};
	~RenderPassComponent() {};

	bool m_UseMultiFrames = false;

	size_t m_RenderTargetCount = 0;
	TextureDataDesc m_RenderTargetDesc = {};
	std::vector<TextureDataComponent*> m_RenderTargets;
	TextureDataComponent* m_DepthStencilRenderTarget;

	GraphicsPipelineDesc m_GraphicsPipelineDesc = {};

	ICommandQueue m_CommandQueue;
	std::vector<ICommandList*> m_CommandLists;

	size_t m_CurrentFrame = 0;

	std::vector<ISemaphore*> m_waitSemaphores;
	std::vector<ISemaphore*> m_singalSemaphores;
	std::vector<IFence*> m_Fences;
};

class ShaderProgramComponent : public InnoComponent
{
public:
	ShaderProgramComponent() {};
	~ShaderProgramComponent() {};

	ShaderFilePaths m_ShaderFilePaths;
};

enum class GPUBufferAccessibility { ReadOnly = 1, WriteOnly = 2, ReadWrite = ReadOnly | WriteOnly };

struct GPUBufferDataComponent
{
	GPUBufferAccessibility m_GPUBufferAccessibility = GPUBufferAccessibility::ReadOnly;
	size_t m_Size = 0;
	size_t m_BindingPoint = 0;
};

class IRenderingServer
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingServer);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool Update() = 0;
	virtual bool Render() = 0;
	virtual bool Present() = 0;
	virtual bool Terminate() = 0;

	virtual ObjectStatus GetStatus() = 0;

	virtual MeshDataComponent* AddMeshDataComponent(const char* name = "") = 0;
	virtual TextureDataComponent* AddTextureDataComponent(const char* name = "") = 0;
	virtual MaterialDataComponent* AddMaterialDataComponent(const char* name = "") = 0;
	virtual RenderPassComponent* AddRenderPassComponent(const char* name = "") = 0;
	virtual ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") = 0;
	virtual GPUBufferDataComponent* AddGPUBufferDataComponent(const char* name = "") = 0;

	virtual bool InitializeMeshDataComponent(MeshDataComponent* rhs) = 0;
	virtual bool InitializeTextureDataComponent(TextureDataComponent* rhs) = 0;
	virtual bool InitializeMaterialDataComponent(MaterialDataComponent* rhs) = 0;
	virtual bool InitializeRenderPassComponent(RenderPassComponent* rhs) = 0;
	virtual	bool InitializeShaderProgramComponent(ShaderProgramComponent* rhs) = 0;
	virtual bool InitializeGPUBufferDataComponent(GPUBufferDataComponent* rhs) = 0;

	virtual	bool DeleteMeshDataComponent(MeshDataComponent* rhs) = 0;
	virtual	bool DeleteTextureDataComponent(TextureDataComponent* rhs) = 0;
	virtual	bool DeleteMaterialDataComponent(MaterialDataComponent* rhs) = 0;
	virtual	bool DeleteRenderPassComponent(RenderPassComponent* rhs) = 0;
	virtual	bool DeleteShaderProgramComponent(ShaderProgramComponent* rhs) = 0;

	virtual void RegisterMeshDataComponent(MeshDataComponent* rhs) = 0;
	virtual void RegisterMaterialDataComponent(MaterialDataComponent* rhs) = 0;

	virtual MeshDataComponent* GetMeshDataComponent(MeshShapeType meshShapeType) = 0;
	virtual TextureDataComponent* GetTextureDataComponent(TextureUsageType textureUsageType) = 0;
	virtual TextureDataComponent* GetTextureDataComponent(WorldEditorIconType iconType) = 0;

	virtual bool UploadGPUBufferDataComponentImpl(GPUBufferDataComponent* rhs, const void* GPUBufferValue) = 0;

	template<typename T>
	void UploadGPUBufferDataComponent(GPUBufferDataComponent* rhs, const T* GPUBufferValue)
	{
		UploadGPUBufferDataComponentImpl(rhs, GPUBufferValue);
	}

	template<typename T>
	void UploadGPUBufferDataComponent(GPUBufferDataComponent* rhs, const std::vector<T>& GPUBufferValue)
	{
		UploadGPUBufferDataComponentImpl(rhs, &GPUBufferValue[0]);
	}

	virtual bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex);
	virtual bool BindRenderPassComponent(RenderPassComponent * rhs) = 0;
	virtual bool CleanRenderTargets(RenderPassComponent * rhs) = 0;
	virtual bool BindGPUBuffer(ShaderType shaderType, GPUBufferAccessibility accessibility, GPUBufferDataComponent* GPUBufferDataComponent, size_t startOffset, size_t range) = 0;
	virtual bool BindShaderProgramComponent(ShaderProgramComponent* rhs) = 0;
	virtual bool BindMaterialDataComponent(MaterialDataComponent* rhs) = 0;
	virtual bool DispatchDrawCall(MeshDataComponent* rhs) = 0;
	virtual bool UnbindMaterialDataComponent(RenderPassComponent* rhs) = 0;
	virtual bool CommandListEnd(RenderPassComponent* rhs, size_t frameIndex) = 0;
	virtual bool ExecuteCommandList(RenderPassComponent* rhs, size_t frameIndex) = 0;
	virtual bool waitForFrame(RenderPassComponent* rhs, size_t frameIndex) = 0;

	virtual bool CopyDepthBuffer(RenderPassComponent* src, RenderPassComponent* dest) = 0;
	virtual bool CopyStencilBuffer(RenderPassComponent* src, RenderPassComponent* dest) = 0;
	virtual bool CopyColorBuffer(RenderPassComponent* src, size_t srcIndex, RenderPassComponent* dest, size_t destIndex) = 0;

	virtual vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) = 0;
	virtual std::vector<vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureDataComponent* TDC) = 0;

	virtual bool Resize() = 0;

	virtual bool ReloadShader(RenderPassType renderPassType) = 0;
	virtual bool BakeGIData() = 0;
};
