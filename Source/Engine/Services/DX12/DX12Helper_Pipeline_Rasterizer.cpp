#include "DX12Helper_Pipeline.h"

using namespace Inno;

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
