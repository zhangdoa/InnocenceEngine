#pragma once
#include "ShaderProgramComponent.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"

class DX12ShaderProgramComponent : public ShaderProgramComponent
{
public:
	ID3DBlob* m_VSBuffer = 0;
	ID3DBlob* m_TCSBuffer = 0;
	ID3DBlob* m_TESBuffer = 0;
	ID3DBlob* m_GSBuffer = 0;
	ID3DBlob* m_FSBuffer = 0;
	ID3DBlob* m_CSBuffer = 0;
};