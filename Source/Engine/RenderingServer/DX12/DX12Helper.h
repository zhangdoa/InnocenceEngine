#pragma once
#include "../../Component/DX12TextureDataComponent.h"
#include "../../Component/DX12RenderPassDataComponent.h"
#include "../../Component/DX12ShaderProgramComponent.h"
#include "../IRenderingServer.h"

namespace DX12Helper
{
	template <typename U, typename T>
	bool SetObjectName(U* owner, T* rhs, const char* objectType)
	{
		auto l_Name = std::string(owner->m_componentName.c_str());
		l_Name += "_";
		l_Name += objectType;
		auto l_NameW = std::wstring(l_Name.begin(), l_Name.end());
		auto l_HResult = rhs->SetName(l_NameW.c_str());
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Warning, "DX12RenderingServer: Can't name ", objectType, " with ", l_Name.c_str());
			return false;
		}
		return true;
	}

	ID3D12GraphicsCommandList* BeginSingleTimeCommands(ID3D12Device* device, ID3D12CommandAllocator* globalCommandAllocator);
	bool EndSingleTimeCommands(ID3D12GraphicsCommandList* commandList, ID3D12Device* device, ID3D12CommandQueue* globalCommandQueue);
	ID3D12Resource* CreateUploadHeapBuffer(UINT64 size, ID3D12Device* device);
	ID3D12Resource* CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, ID3D12Device* device, D3D12_CLEAR_VALUE* clearValue = nullptr);

	D3D12_RESOURCE_DESC GetDX12TextureDataDesc(TextureDataDesc textureDataDesc);
	DXGI_FORMAT GetTextureFormat(TextureDataDesc textureDataDesc);
	D3D12_RESOURCE_DIMENSION GetTextureDimension(TextureDataDesc textureDataDesc);
	D3D12_FILTER GetFilterMode(TextureFilterMethod textureFilterMethod);
	D3D12_TEXTURE_ADDRESS_MODE GetWrapMode(TextureWrapMethod textureWrapMethod);
	unsigned int GetTextureMipLevels(TextureDataDesc textureDataDesc);
	D3D12_RESOURCE_FLAGS GetTextureBindFlags(TextureDataDesc textureDataDesc);
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc(TextureDataDesc textureDataDesc, D3D12_RESOURCE_DESC D3D12TextureDesc);
	D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(TextureDataDesc textureDataDesc, D3D12_RESOURCE_DESC D3D12TextureDesc);
	D3D12_RENDER_TARGET_VIEW_DESC GetRTVDesc(TextureDataDesc textureDataDesc);
	D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDesc(TextureDataDesc textureDataDesc, DepthStencilDesc DSDesc);

	bool ReserveRenderTargets(DX12RenderPassDataComponent* DX12RPDC, IRenderingServer* renderingServer);
	bool CreateRenderTargets(DX12RenderPassDataComponent* DX12RPDC, IRenderingServer* renderingServer);
	bool CreateResourcesBinder(DX12RenderPassDataComponent * DX12RPDC, IRenderingServer* renderingServer);
	bool CreateViews(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device);
	bool CreateRootSignature(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device);
	bool CreatePSO(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device);
	bool CreateCommandQueue(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device);
	bool CreateCommandAllocators(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device);
	bool CreateCommandLists(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device);
	bool CreateSyncPrimitives(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device);

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
	bool GenerateSamplerStateDesc(SamplerDesc samplerDesc, DX12PipelineStateObject* PSO);

	bool LoadShaderFile(ID3D10Blob** rhs, ShaderType shaderType, const ShaderFilePath& shaderFilePath);
}