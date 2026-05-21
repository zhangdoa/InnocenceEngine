#include "DX12Helper_Pipeline.h"

using namespace Inno;

bool DX12Helper::GenerateViewportStateDesc(ViewportDesc viewportDesc, DX12PipelineStateObject* PSO)
{
	PSO->m_Viewport.Width = viewportDesc.m_Width;
	PSO->m_Viewport.Height = viewportDesc.m_Height;
	PSO->m_Viewport.MinDepth = viewportDesc.m_MinDepth;
	PSO->m_Viewport.MaxDepth = viewportDesc.m_MaxDepth;
	PSO->m_Viewport.TopLeftX = viewportDesc.m_OriginX;
	PSO->m_Viewport.TopLeftY = viewportDesc.m_OriginY;

	PSO->m_Scissor.left = 0;
	PSO->m_Scissor.top = 0;
	PSO->m_Scissor.right = (uint64_t)PSO->m_Viewport.Width;
	PSO->m_Scissor.bottom = (uint64_t)PSO->m_Viewport.Height;

	return true;
}
