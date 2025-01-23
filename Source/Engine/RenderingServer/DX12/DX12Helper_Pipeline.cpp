#include "DX12Helper_Pipeline.h"
#include "DX12Helper_Common.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/IOService.h"
#include "DX12RenderingServer.h"

#include "../../Engine.h"

using namespace Inno;

namespace Inno
{
	namespace DX12Helper
	{
#ifdef USE_DXIL
		const char* m_shaderRelativePath = "..//Res//Shaders//DXIL//";
#else
		const wchar_t* m_shaderRelativePath = L"..//Res//Shaders//HLSL//";
#endif
	}
}

D3D12_DESCRIPTOR_HEAP_DESC DX12Helper::GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, wchar_t* name, bool shaderVisible)
{
	D3D12_DESCRIPTOR_HEAP_DESC l_Desc = {};
	l_Desc.Type = type;
	l_Desc.NumDescriptors = numDescriptors;
	l_Desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	l_Desc.NodeMask = 0;
	return l_Desc;
}

D3D12_DESCRIPTOR_RANGE_TYPE DX12Helper::GetDescriptorRangeType(DX12RenderPassComponent* DX12RenderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc)
{
	if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
		return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

	if (resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
	{
		if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
		{
			if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			else if (resourceBinderLayoutDesc.m_ResourceAccessibility.CanWrite())
				return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}
		else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
		{
			if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}
	}
	else if (resourceBinderLayoutDesc.m_BindingAccessibility.CanWrite())
	{
		if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer
		|| resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
			return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	}

	Log(Error, DX12RenderPassComp->m_InstanceName, "Trying to create RootSignature with GPUResourceType that shouldn't/can't be bound.");
	return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}

void Inno::DX12Helper::CreateInputLayout(Inno::DX12PipelineStateObject* PSO)
{
    static D3D12_INPUT_ELEMENT_DESC l_polygonLayout[6];

    l_polygonLayout[0].SemanticName = "POSITION";
    l_polygonLayout[0].SemanticIndex = 0;
    l_polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    l_polygonLayout[0].InputSlot = 0;
    l_polygonLayout[0].AlignedByteOffset = 0;
    l_polygonLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    l_polygonLayout[0].InstanceDataStepRate = 0;

    l_polygonLayout[1].SemanticName = "NORMAL";
    l_polygonLayout[1].SemanticIndex = 0;
    l_polygonLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    l_polygonLayout[1].InputSlot = 0;
    l_polygonLayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    l_polygonLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    l_polygonLayout[1].InstanceDataStepRate = 0;

    l_polygonLayout[2].SemanticName = "TANGENT";
    l_polygonLayout[2].SemanticIndex = 0;
    l_polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    l_polygonLayout[2].InputSlot = 0;
    l_polygonLayout[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    l_polygonLayout[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    l_polygonLayout[2].InstanceDataStepRate = 0;

    l_polygonLayout[3].SemanticName = "TEXCOORD";
    l_polygonLayout[3].SemanticIndex = 0;
    l_polygonLayout[3].Format = DXGI_FORMAT_R32G32_FLOAT;
    l_polygonLayout[3].InputSlot = 0;
    l_polygonLayout[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    l_polygonLayout[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    l_polygonLayout[3].InstanceDataStepRate = 0;

    l_polygonLayout[4].SemanticName = "PAD_A";
    l_polygonLayout[4].SemanticIndex = 0;
    l_polygonLayout[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    l_polygonLayout[4].InputSlot = 0;
    l_polygonLayout[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    l_polygonLayout[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    l_polygonLayout[4].InstanceDataStepRate = 0;

    l_polygonLayout[5].SemanticName = "SV_InstanceID";
    l_polygonLayout[5].SemanticIndex = 0;
    l_polygonLayout[5].Format = DXGI_FORMAT_R32_UINT;
    l_polygonLayout[5].InputSlot = 0;
    l_polygonLayout[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    l_polygonLayout[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    l_polygonLayout[5].InstanceDataStepRate = 0;

    uint32_t l_numElements = sizeof(l_polygonLayout) / sizeof(l_polygonLayout[0]);
    PSO->m_GraphicsPSODesc.InputLayout = { l_polygonLayout, l_numElements };
}

bool DX12Helper::CreateShaderPrograms(DX12RenderPassComponent* DX12RenderPassComp)
{
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(DX12RenderPassComp->m_PipelineStateObject);
	auto l_DX12SPC = reinterpret_cast<DX12ShaderProgramComponent*>(DX12RenderPassComp->m_ShaderProgram);
	
	if (!l_DX12SPC || !l_PSO)
	{
		Log(Verbose, "Skipping creating ShaderPrograms for ", DX12RenderPassComp->m_InstanceName.c_str());
		return true;
	}

	if (DX12RenderPassComp->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
#ifdef USE_DXIL
		if (l_DX12SPC->m_VSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_VSBytecode;
			l_VSBytecode.pShaderBytecode = &l_DX12SPC->m_VSBuffer[0];
			l_VSBytecode.BytecodeLength = l_DX12SPC->m_VSBuffer.size();
			l_PSO->m_GraphicsPSODesc.VS = l_VSBytecode;
		}
		if (l_DX12SPC->m_HSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_HSBytecode;
			l_HSBytecode.pShaderBytecode = &l_DX12SPC->m_HSBuffer[0];
			l_HSBytecode.BytecodeLength = l_DX12SPC->m_HSBuffer.size();
			l_PSO->m_GraphicsPSODesc.HS = l_HSBytecode;
		}
		if (l_DX12SPC->m_DSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_DSBytecode;
			l_DSBytecode.pShaderBytecode = &l_DX12SPC->m_DSBuffer[0];
			l_DSBytecode.BytecodeLength = l_DX12SPC->m_DSBuffer.size();
			l_PSO->m_GraphicsPSODesc.DS = l_DSBytecode;
		}
		if (l_DX12SPC->m_GSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_GSBytecode;
			l_GSBytecode.pShaderBytecode = &l_DX12SPC->m_GSBuffer[0];
			l_GSBytecode.BytecodeLength = l_DX12SPC->m_GSBuffer.size();
			l_PSO->m_GraphicsPSODesc.GS = l_GSBytecode;
		}
		if (l_DX12SPC->m_PSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_PSBytecode;
			l_PSBytecode.pShaderBytecode = &l_DX12SPC->m_PSBuffer[0];
			l_PSBytecode.BytecodeLength = l_DX12SPC->m_PSBuffer.size();
			l_PSO->m_GraphicsPSODesc.PS = l_PSBytecode;
		}
#else
		if (l_DX12SPC->m_VSBuffer)
		{
			D3D12_SHADER_BYTECODE l_VSBytecode;
			l_VSBytecode.pShaderBytecode = l_DX12SPC->m_VSBuffer->GetBufferPointer();
			l_VSBytecode.BytecodeLength = l_DX12SPC->m_VSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.VS = l_VSBytecode;
		}
		if (l_DX12SPC->m_HSBuffer)
		{
			D3D12_SHADER_BYTECODE l_HSBytecode;
			l_HSBytecode.pShaderBytecode = l_DX12SPC->m_HSBuffer->GetBufferPointer();
			l_HSBytecode.BytecodeLength = l_DX12SPC->m_HSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.HS = l_HSBytecode;
		}
		if (l_DX12SPC->m_DSBuffer)
		{
			D3D12_SHADER_BYTECODE l_DSBytecode;
			l_DSBytecode.pShaderBytecode = l_DX12SPC->m_DSBuffer->GetBufferPointer();
			l_DSBytecode.BytecodeLength = l_DX12SPC->m_DSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.DS = l_DSBytecode;
		}
		if (l_DX12SPC->m_GSBuffer)
		{
			D3D12_SHADER_BYTECODE l_GSBytecode;
			l_GSBytecode.pShaderBytecode = l_DX12SPC->m_GSBuffer->GetBufferPointer();
			l_GSBytecode.BytecodeLength = l_DX12SPC->m_GSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.GS = l_GSBytecode;
		}
		if (l_DX12SPC->m_PSBuffer)
		{
			D3D12_SHADER_BYTECODE l_PSBytecode;
			l_PSBytecode.pShaderBytecode = l_DX12SPC->m_PSBuffer->GetBufferPointer();
			l_PSBytecode.BytecodeLength = l_DX12SPC->m_PSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.PS = l_PSBytecode;
		}
#endif
	}
	else
	{
#ifdef USE_DXIL
		if (l_DX12SPC->m_CSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_CSBytecode;
			l_CSBytecode.pShaderBytecode = &l_DX12SPC->m_CSBuffer[0];
			l_CSBytecode.BytecodeLength = l_DX12SPC->m_CSBuffer.size();
			l_PSO->m_ComputePSODesc.CS = l_CSBytecode;
		}
#else
		if (l_DX12SPC->m_CSBuffer)
		{
			D3D12_SHADER_BYTECODE l_CSBytecode;
			l_CSBytecode.pShaderBytecode = l_DX12SPC->m_CSBuffer->GetBufferPointer();
			l_CSBytecode.BytecodeLength = l_DX12SPC->m_CSBuffer->GetBufferSize();
			l_PSO->m_ComputePSODesc.CS = l_CSBytecode;
		}
#endif
	}

	return true;
}


D3D12_COMPARISON_FUNC DX12Helper::GetComparisionFunction(ComparisionFunction comparisionFunction)
{
	D3D12_COMPARISON_FUNC l_result;

	switch (comparisionFunction)
	{
	case ComparisionFunction::Never: l_result = D3D12_COMPARISON_FUNC_NEVER;
		break;
	case ComparisionFunction::Less: l_result = D3D12_COMPARISON_FUNC_LESS;
		break;
	case ComparisionFunction::Equal: l_result = D3D12_COMPARISON_FUNC_EQUAL;
		break;
	case ComparisionFunction::LessEqual: l_result = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		break;
	case ComparisionFunction::Greater: l_result = D3D12_COMPARISON_FUNC_GREATER;
		break;
	case ComparisionFunction::NotEqual: l_result = D3D12_COMPARISON_FUNC_NOT_EQUAL;
		break;
	case ComparisionFunction::GreaterEqual: l_result = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		break;
	case ComparisionFunction::Always: l_result = D3D12_COMPARISON_FUNC_ALWAYS;
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_STENCIL_OP DX12Helper::GetStencilOperation(StencilOperation stencilOperation)
{
	D3D12_STENCIL_OP l_result;

	switch (stencilOperation)
	{
	case StencilOperation::Keep: l_result = D3D12_STENCIL_OP_KEEP;
		break;
	case StencilOperation::Zero: l_result = D3D12_STENCIL_OP_ZERO;
		break;
	case StencilOperation::Replace: l_result = D3D12_STENCIL_OP_REPLACE;
		break;
	case StencilOperation::IncreaseSat: l_result = D3D12_STENCIL_OP_INCR_SAT;
		break;
	case StencilOperation::DecreaseSat: l_result = D3D12_STENCIL_OP_DECR_SAT;
		break;
	case StencilOperation::Invert: l_result = D3D12_STENCIL_OP_INVERT;
		break;
	case StencilOperation::Increase: l_result = D3D12_STENCIL_OP_INCR;
		break;
	case StencilOperation::Decrease: l_result = D3D12_STENCIL_OP_DECR;
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_BLEND DX12Helper::GetBlendFactor(BlendFactor blendFactor)
{
	D3D12_BLEND l_result;

	switch (blendFactor)
	{
	case BlendFactor::Zero: l_result = D3D12_BLEND_ZERO;
		break;
	case BlendFactor::One: l_result = D3D12_BLEND_ONE;
		break;
	case BlendFactor::SrcColor: l_result = D3D12_BLEND_SRC_COLOR;
		break;
	case BlendFactor::OneMinusSrcColor: l_result = D3D12_BLEND_INV_SRC_COLOR;
		break;
	case BlendFactor::SrcAlpha: l_result = D3D12_BLEND_SRC_ALPHA;
		break;
	case BlendFactor::OneMinusSrcAlpha: l_result = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	case BlendFactor::DestColor: l_result = D3D12_BLEND_DEST_COLOR;
		break;
	case BlendFactor::OneMinusDestColor: l_result = D3D12_BLEND_INV_DEST_COLOR;
		break;
	case BlendFactor::DestAlpha: l_result = D3D12_BLEND_DEST_ALPHA;
		break;
	case BlendFactor::OneMinusDestAlpha: l_result = D3D12_BLEND_INV_DEST_ALPHA;
		break;
	case BlendFactor::Src1Color: l_result = D3D12_BLEND_SRC1_COLOR;
		break;
	case BlendFactor::OneMinusSrc1Color: l_result = D3D12_BLEND_INV_SRC1_COLOR;
		break;
	case BlendFactor::Src1Alpha: l_result = D3D12_BLEND_SRC1_ALPHA;
		break;
	case BlendFactor::OneMinusSrc1Alpha: l_result = D3D12_BLEND_INV_SRC1_ALPHA;
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_BLEND_OP DX12Helper::GetBlendOperation(BlendOperation blendOperation)
{
	D3D12_BLEND_OP l_result;

	switch (blendOperation)
	{
	case BlendOperation::Add: l_result = D3D12_BLEND_OP_ADD;
		break;
	case BlendOperation::Substruct: l_result = D3D12_BLEND_OP_SUBTRACT;
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_PRIMITIVE_TOPOLOGY DX12Helper::GetPrimitiveTopology(PrimitiveTopology primitiveTopology)
{
	D3D12_PRIMITIVE_TOPOLOGY l_result;

	switch (primitiveTopology)
	{
	case PrimitiveTopology::Point: l_result = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	case PrimitiveTopology::Line: l_result = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	case PrimitiveTopology::TriangleList: l_result = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case PrimitiveTopology::TriangleStrip: l_result = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;
	case PrimitiveTopology::Patch: l_result = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST; // @TODO: Don't treat Patch as a primitive topology type due to the API differences
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE DX12Helper::GetPrimitiveTopologyType(PrimitiveTopology primitiveTopology)
{
	D3D12_PRIMITIVE_TOPOLOGY_TYPE l_result;

	switch (primitiveTopology)
	{
	case PrimitiveTopology::Point: l_result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case PrimitiveTopology::Line: l_result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case PrimitiveTopology::TriangleList: l_result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveTopology::TriangleStrip: l_result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveTopology::Patch: l_result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH; // @TODO: Don't treat Patch as a primitive topology type due to the API differences
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_FILL_MODE DX12Helper::GetRasterizerFillMode(RasterizerFillMode rasterizerFillMode)
{
	D3D12_FILL_MODE l_result;

	switch (rasterizerFillMode)
	{
	case RasterizerFillMode::Point: // Not supported
		break;
	case RasterizerFillMode::Wireframe: l_result = D3D12_FILL_MODE_WIREFRAME;
		break;
	case RasterizerFillMode::Solid: l_result = D3D12_FILL_MODE_SOLID;
		break;
	default:
		break;
	}

	return l_result;
}

bool DX12Helper::GenerateDepthStencilStateDesc(DepthStencilDesc DSDesc, DX12PipelineStateObject* PSO)
{
	PSO->m_DepthStencilDesc.DepthEnable = DSDesc.m_DepthEnable;

	PSO->m_DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK(DSDesc.m_AllowDepthWrite);
	PSO->m_DepthStencilDesc.DepthFunc = GetComparisionFunction(DSDesc.m_DepthComparisionFunction);

	PSO->m_DepthStencilDesc.StencilEnable = DSDesc.m_StencilEnable;

	PSO->m_DepthStencilDesc.StencilReadMask = 0xFF;
	if (DSDesc.m_AllowStencilWrite)
	{
		PSO->m_DepthStencilDesc.StencilWriteMask = DSDesc.m_StencilWriteMask;
	}
	else
	{
		PSO->m_DepthStencilDesc.StencilWriteMask = 0x00;
	}

	PSO->m_DepthStencilDesc.FrontFace.StencilFailOp = GetStencilOperation(DSDesc.m_FrontFaceStencilFailOperation);
	PSO->m_DepthStencilDesc.FrontFace.StencilDepthFailOp = GetStencilOperation(DSDesc.m_FrontFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilDesc.FrontFace.StencilPassOp = GetStencilOperation(DSDesc.m_FrontFaceStencilPassOperation);
	PSO->m_DepthStencilDesc.FrontFace.StencilFunc = GetComparisionFunction(DSDesc.m_FrontFaceStencilComparisionFunction);

	PSO->m_DepthStencilDesc.BackFace.StencilFailOp = GetStencilOperation(DSDesc.m_BackFaceStencilFailOperation);
	PSO->m_DepthStencilDesc.BackFace.StencilDepthFailOp = GetStencilOperation(DSDesc.m_BackFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilDesc.BackFace.StencilPassOp = GetStencilOperation(DSDesc.m_BackFaceStencilPassOperation);
	PSO->m_DepthStencilDesc.BackFace.StencilFunc = GetComparisionFunction(DSDesc.m_BackFaceStencilComparisionFunction);

	PSO->m_RasterizerDesc.DepthClipEnable = DSDesc.m_AllowDepthClamp;

	return true;
}

bool DX12Helper::GenerateBlendStateDesc(BlendDesc blendDesc, DX12PipelineStateObject* PSO)
{
	// @TODO: Separate alpha and RGB blend operation
	for (size_t i = 0; i < 8; i++)
	{
		PSO->m_BlendDesc.RenderTarget[i].BlendEnable = blendDesc.m_UseBlend;
		PSO->m_BlendDesc.RenderTarget[i].SrcBlend = GetBlendFactor(blendDesc.m_SourceRGBFactor);
		PSO->m_BlendDesc.RenderTarget[i].DestBlend = GetBlendFactor(blendDesc.m_DestinationRGBFactor);
		PSO->m_BlendDesc.RenderTarget[i].BlendOp = GetBlendOperation(blendDesc.m_BlendOperation);
		PSO->m_BlendDesc.RenderTarget[i].SrcBlendAlpha = GetBlendFactor(blendDesc.m_SourceAlphaFactor);
		PSO->m_BlendDesc.RenderTarget[i].DestBlendAlpha = GetBlendFactor(blendDesc.m_DestinationAlphaFactor);
		PSO->m_BlendDesc.RenderTarget[i].BlendOpAlpha = GetBlendOperation(blendDesc.m_BlendOperation);
		PSO->m_BlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	return true;
}

bool DX12Helper::GenerateRasterizerStateDesc(RasterizerDesc rasterizerDesc, DX12PipelineStateObject* PSO)
{
	PSO->m_RasterizerDesc.FillMode = GetRasterizerFillMode(rasterizerDesc.m_RasterizerFillMode);
	if (rasterizerDesc.m_UseCulling)
	{
		PSO->m_RasterizerDesc.CullMode = rasterizerDesc.m_RasterizerCullMode == RasterizerCullMode::Front ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK;
	}
	else
	{
		PSO->m_RasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	}
	PSO->m_RasterizerDesc.FrontCounterClockwise = (rasterizerDesc.m_RasterizerFaceWinding == RasterizerFaceWinding::CCW);
	PSO->m_RasterizerDesc.DepthBias = 0;
	PSO->m_RasterizerDesc.DepthBiasClamp = 0; // @TODO: Depth Clamp
	PSO->m_RasterizerDesc.SlopeScaledDepthBias = 0.0f;
	PSO->m_RasterizerDesc.MultisampleEnable = rasterizerDesc.m_AllowMultisample;
	PSO->m_RasterizerDesc.AntialiasedLineEnable = false;

	PSO->m_PrimitiveTopology = GetPrimitiveTopology(rasterizerDesc.m_PrimitiveTopology);
	PSO->m_PrimitiveTopologyType = GetPrimitiveTopologyType(rasterizerDesc.m_PrimitiveTopology);

	return true;
}

bool DX12Helper::GenerateViewportStateDesc(ViewportDesc viewportDesc, DX12PipelineStateObject* PSO)
{
	PSO->m_Viewport.Width = viewportDesc.m_Width;
	PSO->m_Viewport.Height = viewportDesc.m_Height;
	PSO->m_Viewport.MinDepth = viewportDesc.m_MinDepth;
	PSO->m_Viewport.MaxDepth = viewportDesc.m_MaxDepth;
	PSO->m_Viewport.TopLeftX = viewportDesc.m_OriginX;
	PSO->m_Viewport.TopLeftY = viewportDesc.m_OriginY;

	// Setup the scissor rect.
	PSO->m_Scissor.left = 0;
	PSO->m_Scissor.top = 0;
	PSO->m_Scissor.right = (uint64_t)PSO->m_Viewport.Width;
	PSO->m_Scissor.bottom = (uint64_t)PSO->m_Viewport.Height;

	return true;
}
#ifdef USE_DXIL
bool DX12Helper::LoadShaderFile(std::vector<char> &rhs, const ShaderFilePath &shaderFilePath)
{
	auto l_path = std::string(m_shaderRelativePath) + shaderFilePath.c_str() + ".dxil";
	rhs = g_Engine->Get<IOService>()->loadFile(l_path.c_str(), IOMode::Binary);
	return true;
}
#else
bool DX12Helper::LoadShaderFile(ID3D10Blob** rhs, ShaderStage shaderStage, const ShaderFilePath& shaderFilePath)
{
	const char* l_shaderTypeName;

	switch (shaderStage)
	{
	case ShaderStage::Vertex: l_shaderTypeName = "vs_6_0";
		break;
	case ShaderStage::Hull: l_shaderTypeName = "hs_6_0";
		break;
	case ShaderStage::Domain: l_shaderTypeName = "ds_6_0";
		break;
	case ShaderStage::Geometry: l_shaderTypeName = "gs_6_0";
		break;
	case ShaderStage::Pixel: l_shaderTypeName = "ps_6_0";
		break;
	case ShaderStage::Compute: l_shaderTypeName = "cs_6_0";
		break;
	default:
		break;
	}

#if defined(INNO_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT l_compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT l_compileFlags = 0;
#endif

	ComPtr<ID3D10Blob> l_errorMessage = 0;
	auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
	auto l_workingDirW = std::wstring(l_workingDir.begin(), l_workingDir.end());
	auto l_shadeFilePathW = std::wstring(shaderFilePath.begin(), shaderFilePath.end());
	auto l_HResult = D3DCompileFromFile((l_workingDirW + m_shaderRelativePath + l_shadeFilePathW).c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", l_shaderTypeName, l_compileFlags, 0, rhs, &l_errorMessage);

	if (FAILED(l_HResult))
	{
		if (l_errorMessage)
		{
			auto l_errorMessagePtr = (char*)(l_errorMessage->GetBufferPointer());
			auto bufferSize = l_errorMessage->GetBufferSize();
			std::vector<char> l_errorMessageVector(bufferSize);
			std::memcpy(l_errorMessageVector.data(), l_errorMessagePtr, bufferSize);

			Log(Error, "", shaderFilePath.c_str(), " compile error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			Log(Error, "Can't find ", shaderFilePath.c_str(), " ", name);
		}
		return false;
	}

	Log(Verbose, "", shaderFilePath.c_str(), " has been compiled.");
	return true;
}
#endif