#pragma once
#include "../../Common/LogService.h"

#include "../../Component/RenderPassComponent.h"
#include "../IRenderingServer.h"

#include "DX12Headers.h"

namespace Inno
{
	namespace DX12Helper
	{
		D3D12_DESCRIPTOR_HEAP_DESC GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, bool shaderVisible);
		void CreateInputLayout(DX12PipelineStateObject* PSO);
		bool LoadGraphicsShaders(RenderPassComponent* RenderPassComp);
		bool LoadComputeShaders(RenderPassComponent* RenderPassComp);
		bool LoadRaytracingShaders(RenderPassComponent* RenderPassComp);

		D3D12_COMPARISON_FUNC GetComparisionFunction(ComparisionFunction comparisionFunction);
		D3D12_STENCIL_OP GetStencilOperation(StencilOperation stencilOperation);
		D3D12_BLEND GetBlendFactor(BlendFactor blendFactor);
		D3D12_BLEND_OP GetBlendOperation(BlendOperation blendOperation);
		D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(PrimitiveTopology primitiveTopology);
		D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(PrimitiveTopology primitiveTopology);
		D3D12_FILL_MODE GetRasterizerFillMode(RasterizerFillMode rasterizerFillMode);

		bool GenerateDepthStencilStateDesc(DepthStencilDesc DSDesc, DX12PipelineStateObject* PSO);
		bool GenerateBlendStateDesc(BlendDesc blendDesc, DX12PipelineStateObject* PSO);
		bool GenerateRasterizerStateDesc(RasterizerDesc rasterizerDesc, DX12PipelineStateObject* PSO);
		bool GenerateViewportStateDesc(ViewportDesc viewportDesc, DX12PipelineStateObject* PSO);

#ifdef USE_DXIL
		bool LoadShaderFile(std::vector<char>& rhs, const ShaderFilePath& shaderFilePath);
#else
		bool LoadShaderFile(ID3D10Blob** rhs, ShaderStage shaderStage, const ShaderFilePath& shaderFilePath);
#endif
	} // namespace DX12Helper
} // namespace Inno