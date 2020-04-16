#pragma once
#include "ShaderProgramComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

class DX12ShaderProgramComponent : public ShaderProgramComponent
{
public:
	ID3DBlob* m_VSBuffer = 0;
	ID3DBlob* m_HSBuffer = 0;
	ID3DBlob* m_DSBuffer = 0;
	ID3DBlob* m_GSBuffer = 0;
	ID3DBlob* m_PSBuffer = 0;
	ID3DBlob* m_CSBuffer = 0;
};