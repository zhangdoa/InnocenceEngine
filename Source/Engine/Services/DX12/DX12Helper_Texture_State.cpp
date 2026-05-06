#include "DX12Helper_Texture.h"

using namespace Inno;

D3D12_RESOURCE_STATES DX12Helper::GetTextureWriteState(TextureDesc textureDesc)
{
	D3D12_RESOURCE_STATES l_result;
	if (textureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else if (textureDesc.Usage == TextureUsage::ComputeOnly)
	{
		l_result = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment
		|| textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}
	else
	{
		l_result = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	if (textureDesc.MipLevels > 1)
	{
		l_result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	return l_result;
}

D3D12_RESOURCE_STATES DX12Helper::GetTextureReadState(TextureDesc textureDesc)
{
	D3D12_RESOURCE_STATES l_result = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_SOURCE;

	if (textureDesc.Usage == TextureUsage::DepthAttachment
		|| textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result |= D3D12_RESOURCE_STATE_DEPTH_READ;
	}

	return l_result;
}
