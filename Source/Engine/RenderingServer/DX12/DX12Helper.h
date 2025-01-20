#pragma once
#include "../../Common/LogService.h"
#include "../../Component/DX12TextureComponent.h"
#include "../../Component/DX12RenderPassComponent.h"
#include "../../Component/DX12ShaderProgramComponent.h"
#include "../IRenderingServer.h"

namespace Inno
{
	namespace DX12Helper
	{
		template <typename U, typename T>
		bool SetObjectName(U *owner, ComPtr<T> rhs, const char *objectType)
		{
			auto l_Name = std::string(owner->m_InstanceName.c_str());
			l_Name += "_";
			l_Name += objectType;
			auto l_NameW = std::wstring(l_Name.begin(), l_Name.end());
			auto l_HResult = rhs->SetName(l_NameW.c_str());
			if (FAILED(l_HResult))
			{
				Log(Warning, "Can't name ", objectType, " with ", l_Name.c_str());
				return false;
			}
			return true;
		}

		ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_QUEUE_DESC * commandQueueDesc, ComPtr<ID3D12Device> device, const wchar_t *name = L"");
		ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12Device> device, const wchar_t *name = L"");
		ComPtr<ID3D12GraphicsCommandList> CreateCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandAllocator> commandAllocator, const wchar_t *name = L"");
		ComPtr<ID3D12GraphicsCommandList> CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandAllocator> commandAllocator);
		bool ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> commandQueue);
		
		ComPtr<ID3D12Resource> CreateUploadHeapBuffer(D3D12_RESOURCE_DESC *resourceDesc, ComPtr<ID3D12Device> device, const char *name = "");
		ComPtr<ID3D12Resource> CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC *resourceDesc, ComPtr<ID3D12Device> device, D3D12_CLEAR_VALUE *clearValue = nullptr, const char *name = "");
		ComPtr<ID3D12Resource> CreateReadBackHeapBuffer(UINT64 size, ComPtr<ID3D12Device> device, const char *name = "");

		D3D12_RESOURCE_DESC GetDX12TextureDesc(TextureDesc textureDesc);
		DXGI_FORMAT GetTextureFormat(TextureDesc textureDesc);
		D3D12_RESOURCE_DIMENSION GetTextureDimension(TextureDesc textureDesc);
		D3D12_FILTER GetFilterMode(TextureFilterMethod minFilterMethod, TextureFilterMethod magFilterMethod);
		D3D12_TEXTURE_ADDRESS_MODE GetWrapMode(TextureWrapMethod textureWrapMethod);
		uint32_t GetTextureMipLevels(TextureDesc textureDesc);
		D3D12_RESOURCE_FLAGS GetTextureBindFlags(TextureDesc textureDesc);
		uint32_t GetTexturePixelDataSize(TextureDesc textureDesc);
		D3D12_RESOURCE_STATES GetTextureWriteState(TextureDesc textureDesc);
		D3D12_RESOURCE_STATES GetTextureReadState(TextureDesc textureDesc);
		D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc(TextureDesc textureDesc, D3D12_RESOURCE_DESC D3D12TextureDesc, uint32_t mostDetailedMip);
		D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(TextureDesc textureDesc, D3D12_RESOURCE_DESC D3D12TextureDesc, uint32_t mipSlice);
		D3D12_RENDER_TARGET_VIEW_DESC GetRTVDesc(TextureDesc textureDesc);
		D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDesc(TextureDesc textureDesc, bool stencilEnable);

		bool CreateRootSignature(DX12RenderPassComponent *DX12RenderPassComp, ComPtr<ID3D12Device> device);
        bool CreatePSO(DX12RenderPassComponent* DX12RenderPassComp, ComPtr<ID3D12Device> device);
        void CreateInputLayout(DX12PipelineStateObject* PSO);
		bool CreateShaderPrograms(DX12RenderPassComponent *DX12RenderPassComp);
		bool CreateFenceEvents(DX12RenderPassComponent *DX12RenderPassComp);

		D3D12_COMPARISON_FUNC GetComparisionFunction(ComparisionFunction comparisionFunction);
		D3D12_STENCIL_OP GetStencilOperation(StencilOperation stencilOperation);
		D3D12_BLEND GetBlendFactor(BlendFactor blendFactor);
		D3D12_BLEND_OP GetBlendOperation(BlendOperation blendOperation);
		D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(PrimitiveTopology primitiveTopology);
		D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(PrimitiveTopology primitiveTopology);
		D3D12_FILL_MODE GetRasterizerFillMode(RasterizerFillMode rasterizerFillMode);

		bool GenerateDepthStencilStateDesc(DepthStencilDesc DSDesc, DX12PipelineStateObject *PSO);
		bool GenerateBlendStateDesc(BlendDesc blendDesc, DX12PipelineStateObject *PSO);
		bool GenerateRasterizerStateDesc(RasterizerDesc rasterizerDesc, DX12PipelineStateObject *PSO);
		bool GenerateViewportStateDesc(ViewportDesc viewportDesc, DX12PipelineStateObject *PSO);

#ifdef USE_DXIL
		bool LoadShaderFile(std::vector<char> &rhs, const ShaderFilePath &shaderFilePath);
#else
		bool LoadShaderFile(ID3D10Blob **rhs, ShaderStage shaderStage, const ShaderFilePath &shaderFilePath);
#endif
	} // namespace DX12Helper
} // namespace Inno