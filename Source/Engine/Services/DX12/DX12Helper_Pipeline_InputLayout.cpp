#include "DX12Helper_Pipeline.h"

using namespace Inno;

void DX12Helper::CreateInputLayout(DX12PipelineStateObject* PSO)
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
