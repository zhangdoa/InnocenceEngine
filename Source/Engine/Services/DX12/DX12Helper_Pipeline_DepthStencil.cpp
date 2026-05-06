#include "DX12Helper_Pipeline.h"

using namespace Inno;

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
