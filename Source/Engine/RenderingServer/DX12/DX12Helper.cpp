#include "DX12Helper.h"
#include "../../Core/InnoLogger.h"
#include "DX12RenderingServer.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace DX12Helper
{
	UINT GetMipLevels(TextureDesc textureDesc);

	const wchar_t* m_shaderRelativePath = L"Res//Shaders//HLSL//";
}

ID3D12GraphicsCommandList* DX12Helper::BeginSingleTimeCommands(ID3D12Device* device, ID3D12CommandAllocator* globalCommandAllocator)
{
	ID3D12GraphicsCommandList* l_commandList;

	// Create a basic command list.
	auto l_HResult = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, globalCommandAllocator, NULL, IID_PPV_ARGS(&l_commandList));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create command list!");
		return nullptr;
	}

	return l_commandList;
}

bool DX12Helper::EndSingleTimeCommands(ID3D12GraphicsCommandList* commandList, ID3D12Device* device, ID3D12CommandQueue* globalCommandQueue)
{
	auto l_HResult = commandList->Close();
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't close the command list for single command!");
	}

	ID3D12Fence1* l_uploadFinishFence;
	l_HResult = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&l_uploadFinishFence));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create fence for single command!");
	}

	auto l_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (l_fenceEvent == nullptr)
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create fence event for single command!");
	}

	ID3D12CommandList* ppCommandLists[] = { commandList };
	globalCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	globalCommandQueue->Signal(l_uploadFinishFence, 1);
	l_uploadFinishFence->SetEventOnCompletion(1, l_fenceEvent);
	WaitForSingleObject(l_fenceEvent, INFINITE);
	CloseHandle(l_fenceEvent);

	commandList->Release();

	return true;
}

ID3D12Resource* DX12Helper::CreateUploadHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, ID3D12Device* device, const char* name)
{
	ID3D12Resource* l_uploadHeapBuffer;

	auto l_HResult = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&l_uploadHeapBuffer));

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create upload heap buffer!");
		return nullptr;
	}

	return l_uploadHeapBuffer;
}

ID3D12Resource* DX12Helper::CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, ID3D12Device* device, D3D12_CLEAR_VALUE* clearValue, const char* name)
{
	ID3D12Resource* l_defaultHeapBuffer;

	auto l_HResult = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		clearValue,
		IID_PPV_ARGS(&l_defaultHeapBuffer));

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create default heap buffer!");
		return false;
	}

	return l_defaultHeapBuffer;
}

ID3D12Resource * DX12Helper::CreateReadBackHeapBuffer(UINT64 size, ID3D12Device * device, const char * name)
{
	ID3D12Resource* l_readBackHeapBuffer;

	auto l_HResult = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&l_readBackHeapBuffer));

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create read-back heap buffer!");
		return nullptr;
	}

	return l_readBackHeapBuffer;
}

D3D12_RESOURCE_DESC DX12Helper::GetDX12TextureDesc(TextureDesc textureDesc)
{
	D3D12_RESOURCE_DESC l_result = {};

	l_result.Width = textureDesc.Width;
	l_result.Height = textureDesc.Height;
	switch (textureDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D:
		l_result.DepthOrArraySize = 1;
		break;
	case TextureSamplerType::Sampler2D:
		l_result.DepthOrArraySize = 1;
		break;
	case TextureSamplerType::Sampler3D:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler1DArray:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler2DArray:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::SamplerCubemap:
		l_result.DepthOrArraySize = 6;
		break;
	default:
		break;
	}

	if (textureDesc.CPUAccessibility != Accessibility::Immutable)
	{
		l_result.MipLevels = 1;
		l_result.Format = DXGI_FORMAT_UNKNOWN;
		l_result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		l_result.SampleDesc.Count = 1;
		l_result.SampleDesc.Quality = 0;
		l_result.Flags = D3D12_RESOURCE_FLAG_NONE;
		l_result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	}
	else
	{
		l_result.MipLevels = GetTextureMipLevels(textureDesc);
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.SampleDesc.Count = 1;
		l_result.SampleDesc.Quality = 0;
		l_result.Dimension = GetTextureDimension(textureDesc);
		l_result.Flags = GetTextureBindFlags(textureDesc);
	}

	return l_result;
}

DXGI_FORMAT DX12Helper::GetTextureFormat(TextureDesc textureDesc)
{
	DXGI_FORMAT l_internalFormat = DXGI_FORMAT_UNKNOWN;

	if (textureDesc.IsSRGB)
	{
		l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else if (textureDesc.UsageType == TextureUsageType::DepthAttachment)
	{
		l_internalFormat = DXGI_FORMAT_R32_TYPELESS;
	}
	else if (textureDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		l_internalFormat = DXGI_FORMAT_R24G8_TYPELESS;
	}
	else
	{
		if (textureDesc.PixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			default: break;
			}
		}
		if (textureDesc.PixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_UINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_SINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			default: break;
			}
		}
	}

	return l_internalFormat;
}

D3D12_RESOURCE_DIMENSION DX12Helper::GetTextureDimension(TextureDesc textureDesc)
{
	D3D12_RESOURCE_DIMENSION l_result;

	switch (textureDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case TextureSamplerType::Sampler2D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case TextureSamplerType::Sampler3D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		break;
	case TextureSamplerType::Sampler1DArray: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case TextureSamplerType::Sampler2DArray: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case TextureSamplerType::SamplerCubemap: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_FILTER DX12Helper::GetFilterMode(TextureFilterMethod minFilterMethod, TextureFilterMethod magFilterMethod)
{
	D3D12_FILTER l_result;

	if (minFilterMethod == TextureFilterMethod::Nearest)
	{
		if (magFilterMethod == TextureFilterMethod::Nearest)
		{
			l_result = D3D12_FILTER_MIN_MAG_MIP_POINT;
		}
		else
		{
			l_result = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		}
	}
	else
	{
		if (magFilterMethod == TextureFilterMethod::Nearest)
		{
			l_result = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		}
		else
		{
			l_result = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		}
	}

	return l_result;
}

D3D12_TEXTURE_ADDRESS_MODE DX12Helper::GetWrapMode(TextureWrapMethod textureWrapMethod)
{
	D3D12_TEXTURE_ADDRESS_MODE l_result;

	switch (textureWrapMethod)
	{
	case TextureWrapMethod::Edge: l_result = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		break;
	case TextureWrapMethod::Repeat: l_result = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		break;
	case TextureWrapMethod::Border: l_result = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		break;
	default:
		break;
	}

	return l_result;
}

uint32_t DX12Helper::GetTextureMipLevels(TextureDesc textureDesc)
{
	uint32_t textureMipLevels = 1;
	if (textureDesc.UseMipMap)
	{
		textureMipLevels = 0;
	}

	return textureMipLevels;
}

D3D12_RESOURCE_FLAGS DX12Helper::GetTextureBindFlags(TextureDesc textureDesc)
{
	D3D12_RESOURCE_FLAGS l_result = {};

	if (textureDesc.UsageType == TextureUsageType::ColorAttachment)
	{
		l_result = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	else if (textureDesc.UsageType == TextureUsageType::DepthAttachment)
	{
		l_result = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	else if (textureDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		l_result = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	else if (textureDesc.UsageType == TextureUsageType::RawImage)
	{
		l_result = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return l_result;
}

uint32_t DX12Helper::GetTexturePixelDataSize(TextureDesc textureDesc)
{
	uint32_t l_singlePixelSize;

	switch (textureDesc.PixelDataType)
	{
	case TexturePixelDataType::UBYTE:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SBYTE:l_singlePixelSize = 1; break;
	case TexturePixelDataType::USHORT:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SSHORT:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UINT8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SINT8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::UINT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SINT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UINT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::SINT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::FLOAT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::FLOAT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::DOUBLE:l_singlePixelSize = 8; break;
	}

	uint32_t l_channelSize;
	switch (textureDesc.PixelDataFormat)
	{
	case TexturePixelDataFormat::R:l_channelSize = 1; break;
	case TexturePixelDataFormat::RG:l_channelSize = 2; break;
	case TexturePixelDataFormat::RGB:l_channelSize = 3; break;
	case TexturePixelDataFormat::RGBA:l_channelSize = 4; break;
	case TexturePixelDataFormat::Depth:l_channelSize = 1; break;
	case TexturePixelDataFormat::DepthStencil:l_channelSize = 1; break;
	}

	return l_singlePixelSize * l_channelSize;
}

D3D12_RESOURCE_STATES DX12Helper::GetTextureWriteState(TextureDesc textureDesc)
{
	D3D12_RESOURCE_STATES l_result;

	if (textureDesc.UsageType == TextureUsageType::ColorAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else if (textureDesc.UsageType == TextureUsageType::DepthAttachment
		|| textureDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}
	else if (textureDesc.UsageType == TextureUsageType::RawImage)
	{
		l_result = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else
	{
		l_result = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	return l_result;
}

D3D12_RESOURCE_STATES DX12Helper::GetTextureReadState(TextureDesc textureDesc)
{
	D3D12_RESOURCE_STATES l_result;

	if (textureDesc.UsageType == TextureUsageType::ColorAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	else if (textureDesc.UsageType == TextureUsageType::DepthAttachment
		|| textureDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
	else if (textureDesc.UsageType == TextureUsageType::RawImage)
	{
		l_result = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
	else
	{
		l_result = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	return l_result;
}

UINT DX12Helper::GetMipLevels(TextureDesc textureDesc)
{
	if (textureDesc.UsageType == TextureUsageType::ColorAttachment
		|| textureDesc.UsageType == TextureUsageType::DepthAttachment
		|| textureDesc.UsageType == TextureUsageType::DepthStencilAttachment
		|| textureDesc.UsageType == TextureUsageType::RawImage)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

D3D12_SHADER_RESOURCE_VIEW_DESC DX12Helper::GetSRVDesc(TextureDesc textureDesc, D3D12_RESOURCE_DESC D3D12TextureDesc)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC l_result = {};
	l_result.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (textureDesc.UsageType == TextureUsageType::DepthAttachment)
	{
		l_result.Format = DXGI_FORMAT_R32_FLOAT;
	}
	else if (textureDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		l_result.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	else
	{
		l_result.Format = D3D12TextureDesc.Format;
	}

	switch (textureDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MostDetailedMip = 0;
		l_result.Texture1D.MipLevels = GetMipLevels(textureDesc);
		break;
	case TextureSamplerType::Sampler2D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MostDetailedMip = 0;
		l_result.Texture2D.MipLevels = GetMipLevels(textureDesc);
		break;
	case TextureSamplerType::Sampler3D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MostDetailedMip = 0;
		l_result.Texture3D.MipLevels = GetMipLevels(textureDesc);
		break;
	case TextureSamplerType::Sampler1DArray:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MostDetailedMip = 0;
		l_result.Texture1DArray.MipLevels = GetMipLevels(textureDesc);
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler2DArray:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MostDetailedMip = 0;
		l_result.Texture2DArray.MipLevels = GetMipLevels(textureDesc);
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::SamplerCubemap:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		l_result.TextureCube.MostDetailedMip = 0;
		l_result.TextureCube.MipLevels = GetMipLevels(textureDesc);
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC DX12Helper::GetUAVDesc(TextureDesc textureDesc, D3D12_RESOURCE_DESC D3D12TextureDesc)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_result = {};

	switch (textureDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
		break;
	case TextureSamplerType::Sampler2D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
		break;
	case TextureSamplerType::Sampler3D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MipSlice = 0;
		l_result.Texture3D.WSize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler1DArray:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = 0;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler2DArray:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::SamplerCubemap:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: Use 2D texture array for UAV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_RENDER_TARGET_VIEW_DESC DX12Helper::GetRTVDesc(TextureDesc textureDesc)
{
	D3D12_RENDER_TARGET_VIEW_DESC l_result = {};

	switch (textureDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
		break;
	case TextureSamplerType::Sampler2D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
		break;
	case TextureSamplerType::Sampler3D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MipSlice = 0;
		l_result.Texture3D.WSize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler1DArray:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = 0;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler2DArray:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::SamplerCubemap:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = 6;
		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: Use 2D texture array for RTV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_DEPTH_STENCIL_VIEW_DESC DX12Helper::GetDSVDesc(TextureDesc textureDesc, DepthStencilDesc DSDesc)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC l_result = {};

	if (DSDesc.m_UseStencilBuffer)
	{
		l_result.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		l_result.Format = DXGI_FORMAT_D32_FLOAT;
	}

	switch (textureDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
		break;
	case TextureSamplerType::Sampler2D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
		break;
	case TextureSamplerType::Sampler3D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: Use 2D texture array for DSV of 3D texture.");
		break;
	case TextureSamplerType::Sampler1DArray:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = 0;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler2DArray:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::SamplerCubemap:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = 6;
		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: Use 2D texture array for DSV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

bool DX12Helper::ReserveRenderTargets(DX12RenderPassDataComponent* DX12RPDC, IRenderingServer* renderingServer)
{
	DX12RPDC->m_RenderTargets.reserve(DX12RPDC->m_RenderPassDesc.m_RenderTargetCount);
	for (size_t i = 0; i < DX12RPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		DX12RPDC->m_RenderTargets.emplace_back();
		DX12RPDC->m_RenderTargets[i] = renderingServer->AddTextureDataComponent((std::string(DX12RPDC->m_ComponentName.c_str()) + "_" + std::to_string(i) + "/").c_str());
	}

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " render targets have been allocated.");

	return true;
}

bool DX12Helper::CreateRenderTargets(DX12RenderPassDataComponent* DX12RPDC, IRenderingServer* renderingServer)
{
	auto l_DX12RenderingServer = reinterpret_cast<DX12RenderingServer*>(renderingServer);

	for (size_t i = 0; i < DX12RPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		auto l_TDC = DX12RPDC->m_RenderTargets[i];

		l_TDC->m_textureDesc = DX12RPDC->m_RenderPassDesc.m_RenderTargetDesc;

		l_TDC->m_textureData = nullptr;

		renderingServer->InitializeTextureDataComponent(l_TDC);
	}

	if (DX12RPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		DX12RPDC->m_DepthStencilRenderTarget = renderingServer->AddTextureDataComponent((std::string(DX12RPDC->m_ComponentName.c_str()) + "_DS/").c_str());
		DX12RPDC->m_DepthStencilRenderTarget->m_textureDesc = DX12RPDC->m_RenderPassDesc.m_RenderTargetDesc;

		if (DX12RPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer)
		{
			DX12RPDC->m_DepthStencilRenderTarget->m_textureDesc.UsageType = TextureUsageType::DepthStencilAttachment;
			DX12RPDC->m_DepthStencilRenderTarget->m_textureDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
		}
		else
		{
			DX12RPDC->m_DepthStencilRenderTarget->m_textureDesc.UsageType = TextureUsageType::DepthAttachment;
			DX12RPDC->m_DepthStencilRenderTarget->m_textureDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
		}

		DX12RPDC->m_DepthStencilRenderTarget->m_textureData = { nullptr };

		renderingServer->InitializeTextureDataComponent(DX12RPDC->m_DepthStencilRenderTarget);
	}

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " render targets have been created.");

	return true;
}

bool DX12Helper::CreateResourcesBinder(DX12RenderPassDataComponent * DX12RPDC, IRenderingServer* renderingServer)
{
	auto l_DX12RenderingServer = reinterpret_cast<DX12RenderingServer*>(renderingServer);

	for (size_t i = 0; i < DX12RPDC->m_RenderTargetsResourceBinders.size(); i++)
	{
		DX12RPDC->m_RenderTargetsResourceBinders[i] = DX12RPDC->m_RenderTargets[i]->m_ResourceBinder;
	}

	return true;
}

bool DX12Helper::CreateViews(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device)
{
	if (DX12RPDC->m_RenderPassDesc.m_RenderTargetDesc.UsageType != TextureUsageType::RawImage)
	{
		// Reserve for RTV
		DX12RPDC->m_RTVDescriptorCPUHandles.reserve(DX12RPDC->m_RenderPassDesc.m_RenderTargetCount);
		for (size_t i = 0; i < DX12RPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			DX12RPDC->m_RTVDescriptorCPUHandles.emplace_back();
		}

		// RTV Descriptor Heap
		DX12RPDC->m_RTVDescriptorHeapDesc.NumDescriptors = (uint32_t)DX12RPDC->m_RenderPassDesc.m_RenderTargetCount;
		DX12RPDC->m_RTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		DX12RPDC->m_RTVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		if (DX12RPDC->m_RTVDescriptorHeapDesc.NumDescriptors)
		{
			auto l_HResult = device->CreateDescriptorHeap(&DX12RPDC->m_RTVDescriptorHeapDesc, IID_PPV_ARGS(&DX12RPDC->m_RTVDescriptorHeap));
			if (FAILED(l_HResult))
			{
				InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create DescriptorHeap for RTV!");
				return false;
			}
#ifdef _DEBUG
			SetObjectName(DX12RPDC, DX12RPDC->m_RTVDescriptorHeap, "RTVDescriptorHeap");
#endif // _DEBUG

			DX12RPDC->m_RTVDescriptorCPUHandles[0] = DX12RPDC->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		}

		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " RTV DescriptorHeap has been created.");

		// RTV
		auto l_RTVDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		DX12RPDC->m_RTVDesc = GetRTVDesc(DX12RPDC->m_RenderPassDesc.m_RenderTargetDesc);

		for (size_t i = 1; i < DX12RPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			DX12RPDC->m_RTVDescriptorCPUHandles[i].ptr = DX12RPDC->m_RTVDescriptorCPUHandles[i - 1].ptr + l_RTVDescSize;
		}

		for (size_t i = 0; i < DX12RPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			auto l_ResourceHandle = reinterpret_cast<DX12TextureDataComponent*>(DX12RPDC->m_RenderTargets[i])->m_ResourceHandle;
			device->CreateRenderTargetView(l_ResourceHandle, &DX12RPDC->m_RTVDesc, DX12RPDC->m_RTVDescriptorCPUHandles[i]);
		}

		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " RTV has been created.");
	}

	if (DX12RPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		// DSV Descriptor Heap
		DX12RPDC->m_DSVDescriptorHeapDesc.NumDescriptors = 1;
		DX12RPDC->m_DSVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		DX12RPDC->m_DSVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		auto l_HResult = device->CreateDescriptorHeap(&DX12RPDC->m_DSVDescriptorHeapDesc, IID_PPV_ARGS(&DX12RPDC->m_DSVDescriptorHeap));

		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create DescriptorHeap for DSV!");
			return false;
		}
#ifdef _DEBUG
		SetObjectName(DX12RPDC, DX12RPDC->m_DSVDescriptorHeap, "DSVDescriptorHeap");
#endif // _DEBUG

		DX12RPDC->m_DSVDescriptorCPUHandle = DX12RPDC->m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " DSV DescriptorHeap has been created.");

		// DSV
		auto l_DSVDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		DX12RPDC->m_DSVDesc = GetDSVDesc(DX12RPDC->m_RenderPassDesc.m_RenderTargetDesc, DX12RPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc);

		auto l_ResourceHandle = reinterpret_cast<DX12TextureDataComponent*>(DX12RPDC->m_DepthStencilRenderTarget)->m_ResourceHandle;
		device->CreateDepthStencilView(l_ResourceHandle, &DX12RPDC->m_DSVDesc, DX12RPDC->m_DSVDescriptorCPUHandle);

		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " DSV has been created.");
	}

	return true;
}

bool DX12Helper::CreateRootSignature(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device)
{
	std::vector<CD3DX12_ROOT_PARAMETER1> l_rootParameters(DX12RPDC->m_ResourceBinderLayoutDescs.size());

	size_t l_rootDescriptorTableCount = 0;

	for (size_t i = 0; i < l_rootParameters.size(); i++)
	{
		auto l_resourceBinderLayoutDesc = DX12RPDC->m_ResourceBinderLayoutDescs[i];

		if (l_resourceBinderLayoutDesc.m_IndirectBinding)
		{
			l_rootDescriptorTableCount++;
		}
	}

	std::vector<CD3DX12_DESCRIPTOR_RANGE1> l_rootDescriptorTables(l_rootDescriptorTableCount);

	size_t l_currentTableIndex = 0;

	for (size_t i = 0; i < l_rootParameters.size(); i++)
	{
		auto l_resourceBinderLayoutDesc = DX12RPDC->m_ResourceBinderLayoutDescs[i];

		if (l_resourceBinderLayoutDesc.m_IndirectBinding)
		{
			switch (l_resourceBinderLayoutDesc.m_ResourceBinderType)
			{
			case ResourceBinderType::Sampler: l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, (uint32_t)l_resourceBinderLayoutDesc.m_ResourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
				break;
			case ResourceBinderType::Image:
				if (l_resourceBinderLayoutDesc.m_BinderAccessibility == Accessibility::ReadOnly)
				{
					l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (uint32_t)l_resourceBinderLayoutDesc.m_ResourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
				}
				else
				{
					if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
					{
						InnoLogger::Log(LogLevel::Warning, "DX12RenderingServer: Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
					}
					else
					{
						l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (uint32_t)l_resourceBinderLayoutDesc.m_ResourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
					}
				}
				break;
			case ResourceBinderType::Buffer:
				if (l_resourceBinderLayoutDesc.m_BinderAccessibility == Accessibility::ReadOnly)
				{
					if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
					{
						l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, (uint32_t)l_resourceBinderLayoutDesc.m_ResourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
					}
					else
					{
						l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (uint32_t)l_resourceBinderLayoutDesc.m_ResourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
					}
				}
				else
				{
					if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
					{
						InnoLogger::Log(LogLevel::Warning, "DX12RenderingServer: Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
					}
					else
					{
						l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (uint32_t)l_resourceBinderLayoutDesc.m_ResourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
					}
				}
				break;
			default:
				break;
			}

			l_rootParameters[i].InitAsDescriptorTable(1, &l_rootDescriptorTables[l_currentTableIndex], D3D12_SHADER_VISIBILITY_ALL);

			l_currentTableIndex++;
		}
		else
		{
			switch (l_resourceBinderLayoutDesc.m_ResourceBinderType)
			{
			case ResourceBinderType::Sampler: InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Sampler only could be accessed through a Descriptor table!");
				break;
			case ResourceBinderType::Image: l_rootParameters[i].InitAsShaderResourceView((uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex, 0);
				break;
			case ResourceBinderType::Buffer:
				if (l_resourceBinderLayoutDesc.m_BinderAccessibility == Accessibility::ReadOnly)
				{
					if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
					{
						l_rootParameters[i].InitAsConstantBufferView((uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex, 0);
					}
					else
					{
						l_rootParameters[i].InitAsShaderResourceView((uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex, 0);
					}
				}
				else
				{
					if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
					{
						InnoLogger::Log(LogLevel::Warning, "DX12RenderingServer: Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
					}
					else
					{
						l_rootParameters[i].InitAsUnorderedAccessView((uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex, 0);
					}
				}
				break;
			default:
				break;
			}
		}
	}

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC l_rootSigDesc((uint32_t)l_rootParameters.size(), l_rootParameters.data());
	l_rootSigDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* l_signature = 0;
	ID3DBlob* l_error = 0;

	auto l_HResult = D3D12SerializeVersionedRootSignature(&l_rootSigDesc, &l_signature, &l_error);

	if (FAILED(l_HResult))
	{
		if (l_error)
		{
			auto l_errorMessagePtr = (char*)(l_error->GetBufferPointer());
			auto bufferSize = l_error->GetBufferSize();
			std::vector<char> l_errorMessageVector(bufferSize);
			std::memcpy(l_errorMessageVector.data(), l_errorMessagePtr, bufferSize);
			l_error->Release();

			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " RootSignature serialization error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't serialize RootSignature!");
		}
		return false;
	}

	l_HResult = device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&DX12RPDC->m_RootSignature));
	l_signature->Release();

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create RootSignature!");
		return false;
	}
#ifdef _DEBUG
	SetObjectName(DX12RPDC, DX12RPDC->m_RootSignature, "RootSignature");
#endif // _DEBUG

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " RootSignature has been created.");

	return true;
}

bool DX12Helper::CreatePSO(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device)
{
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(DX12RPDC->m_PipelineStateObject);
	auto l_DX12SPC = reinterpret_cast<DX12ShaderProgramComponent*>(DX12RPDC->m_ShaderProgram);

	if (DX12RPDC->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		GenerateDepthStencilStateDesc(DX12RPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, l_PSO);
		GenerateBlendStateDesc(DX12RPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, l_PSO);
		GenerateRasterizerStateDesc(DX12RPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, l_PSO);
		GenerateViewportStateDesc(DX12RPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, l_PSO);

		l_PSO->m_GraphicsPSODesc.pRootSignature = DX12RPDC->m_RootSignature;

		D3D12_INPUT_ELEMENT_DESC l_polygonLayout[5];
		uint32_t l_numElements;

		// Create the vertex input layout description.
		l_polygonLayout[0].SemanticName = "POSITION";
		l_polygonLayout[0].SemanticIndex = 0;
		l_polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		l_polygonLayout[0].InputSlot = 0;
		l_polygonLayout[0].AlignedByteOffset = 0;
		l_polygonLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		l_polygonLayout[0].InstanceDataStepRate = 0;

		l_polygonLayout[1].SemanticName = "TEXCOORD";
		l_polygonLayout[1].SemanticIndex = 0;
		l_polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		l_polygonLayout[1].InputSlot = 0;
		l_polygonLayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		l_polygonLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		l_polygonLayout[1].InstanceDataStepRate = 0;

		l_polygonLayout[2].SemanticName = "PADA";
		l_polygonLayout[2].SemanticIndex = 0;
		l_polygonLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
		l_polygonLayout[2].InputSlot = 0;
		l_polygonLayout[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		l_polygonLayout[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		l_polygonLayout[2].InstanceDataStepRate = 0;

		l_polygonLayout[3].SemanticName = "NORMAL";
		l_polygonLayout[3].SemanticIndex = 0;
		l_polygonLayout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		l_polygonLayout[3].InputSlot = 0;
		l_polygonLayout[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		l_polygonLayout[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		l_polygonLayout[3].InstanceDataStepRate = 0;

		l_polygonLayout[4].SemanticName = "PADB";
		l_polygonLayout[4].SemanticIndex = 0;
		l_polygonLayout[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		l_polygonLayout[4].InputSlot = 0;
		l_polygonLayout[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		l_polygonLayout[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		l_polygonLayout[4].InstanceDataStepRate = 0;

		// Get a count of the elements in the layout.
		l_numElements = sizeof(l_polygonLayout) / sizeof(l_polygonLayout[0]);
		l_PSO->m_GraphicsPSODesc.InputLayout = { l_polygonLayout, l_numElements };

		if (l_DX12SPC->m_VSBuffer)
		{
			D3D12_SHADER_BYTECODE l_VSBytecode;
			l_VSBytecode.pShaderBytecode = l_DX12SPC->m_VSBuffer->GetBufferPointer();
			l_VSBytecode.BytecodeLength = l_DX12SPC->m_VSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.VS = l_VSBytecode;
		}
		if (l_DX12SPC->m_HSBuffer)
		{
			D3D12_SHADER_BYTECODE l_HSBytecode;
			l_HSBytecode.pShaderBytecode = l_DX12SPC->m_HSBuffer->GetBufferPointer();
			l_HSBytecode.BytecodeLength = l_DX12SPC->m_HSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.HS = l_HSBytecode;
		}
		if (l_DX12SPC->m_DSBuffer)
		{
			D3D12_SHADER_BYTECODE l_DSBytecode;
			l_DSBytecode.pShaderBytecode = l_DX12SPC->m_DSBuffer->GetBufferPointer();
			l_DSBytecode.BytecodeLength = l_DX12SPC->m_DSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.DS = l_DSBytecode;
		}
		if (l_DX12SPC->m_GSBuffer)
		{
			D3D12_SHADER_BYTECODE l_GSBytecode;
			l_GSBytecode.pShaderBytecode = l_DX12SPC->m_GSBuffer->GetBufferPointer();
			l_GSBytecode.BytecodeLength = l_DX12SPC->m_GSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.GS = l_GSBytecode;
		}
		if (l_DX12SPC->m_PSBuffer)
		{
			D3D12_SHADER_BYTECODE l_PSBytecode;
			l_PSBytecode.pShaderBytecode = l_DX12SPC->m_PSBuffer->GetBufferPointer();
			l_PSBytecode.BytecodeLength = l_DX12SPC->m_PSBuffer->GetBufferSize();
			l_PSO->m_GraphicsPSODesc.PS = l_PSBytecode;
		}

		if (DX12RPDC->m_RenderPassDesc.m_RenderTargetDesc.UsageType != TextureUsageType::RawImage)
		{
			l_PSO->m_GraphicsPSODesc.NumRenderTargets = (uint32_t)DX12RPDC->m_RenderPassDesc.m_RenderTargetCount;
			for (size_t i = 0; i < DX12RPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				l_PSO->m_GraphicsPSODesc.RTVFormats[i] = DX12RPDC->m_RTVDesc.Format;
			}
		}

		l_PSO->m_GraphicsPSODesc.DSVFormat = DX12RPDC->m_DSVDesc.Format;
		l_PSO->m_GraphicsPSODesc.DepthStencilState = l_PSO->m_DepthStencilDesc;
		l_PSO->m_GraphicsPSODesc.RasterizerState = l_PSO->m_RasterizerDesc;
		l_PSO->m_GraphicsPSODesc.BlendState = l_PSO->m_BlendDesc;
		l_PSO->m_GraphicsPSODesc.SampleMask = UINT_MAX;
		l_PSO->m_GraphicsPSODesc.PrimitiveTopologyType = l_PSO->m_PrimitiveTopologyType;
		l_PSO->m_GraphicsPSODesc.SampleDesc.Count = 1;

		auto l_HResult = device->CreateGraphicsPipelineState(&l_PSO->m_GraphicsPSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create Graphics PSO!");
			return false;
		}
	}
	else
	{
		l_PSO->m_ComputePSODesc.pRootSignature = DX12RPDC->m_RootSignature;

		if (l_DX12SPC->m_CSBuffer)
		{
			D3D12_SHADER_BYTECODE l_CSBytecode;
			l_CSBytecode.pShaderBytecode = l_DX12SPC->m_CSBuffer->GetBufferPointer();
			l_CSBytecode.BytecodeLength = l_DX12SPC->m_CSBuffer->GetBufferSize();
			l_PSO->m_ComputePSODesc.CS = l_CSBytecode;
		}

		auto l_HResult = device->CreateComputePipelineState(&l_PSO->m_ComputePSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create Compute PSO!");
			return false;
		}
	}

#ifdef _DEBUG
	SetObjectName(DX12RPDC, l_PSO->m_PSO, "PSO");
#endif // _DEBUG

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " PSO has been created.");

	return true;
}

bool DX12Helper::CreateCommandQueue(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device)
{
	auto l_CommandQueue = reinterpret_cast<DX12CommandQueue*>(DX12RPDC->m_CommandQueue);

	// Set up the description of the command queue.
	l_CommandQueue->m_CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	l_CommandQueue->m_CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	l_CommandQueue->m_CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	l_CommandQueue->m_CommandQueueDesc.NodeMask = 0;

	// Create the command queue.
	auto l_HResult = device->CreateCommandQueue(&l_CommandQueue->m_CommandQueueDesc, IID_PPV_ARGS(&l_CommandQueue->m_CommandQueue));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create CommandQueue!");
		return false;
	}
#ifdef _DEBUG
	SetObjectName(DX12RPDC, l_CommandQueue->m_CommandQueue, "CommandQueue");
#endif // _DEBUG

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " CommandQueue has been created.");

	return true;
}

bool DX12Helper::CreateCommandAllocators(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device)
{
	if (DX12RPDC->m_RenderPassDesc.m_UseMultiFrames)
	{
		DX12RPDC->m_CommandAllocators.resize(DX12RPDC->m_RenderPassDesc.m_RenderTargetCount);
	}
	else
	{
		DX12RPDC->m_CommandAllocators.resize(1);
	}

	for (size_t i = 0; i < DX12RPDC->m_CommandAllocators.size(); i++)
	{
		// Create a command allocator.
		auto l_HResult = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&DX12RPDC->m_CommandAllocators[i]));
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create CommandAllocator!");
			return false;
		}
#ifdef _DEBUG
		SetObjectName(DX12RPDC, DX12RPDC->m_CommandAllocators[i], ("CommandAllocator_" + std::to_string(i)).c_str());
#endif // _DEBUG

		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " CommandAllocator has been created.");
	}

	return true;
}

bool DX12Helper::CreateCommandLists(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device)
{
	for (size_t i = 0; i < DX12RPDC->m_CommandLists.size(); i++)
	{
		auto l_CommandList = reinterpret_cast<DX12CommandList*>(DX12RPDC->m_CommandLists[i]);

		auto l_HResult = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, DX12RPDC->m_CommandAllocators[i], NULL, IID_PPV_ARGS(&l_CommandList->m_GraphicsCommandList));
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create CommandList!");
			return false;
		}
#ifdef _DEBUG
		SetObjectName(DX12RPDC, l_CommandList->m_GraphicsCommandList, ("CommandList_" + std::to_string(i)).c_str());
#endif // _DEBUG

		l_CommandList->m_GraphicsCommandList->Close();
	}

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " CommandList has been created.");

	return true;
}

bool DX12Helper::CreateSyncPrimitives(DX12RenderPassDataComponent* DX12RPDC, ID3D12Device* device)
{
	for (size_t i = 0; i < DX12RPDC->m_Fences.size(); i++)
	{
		auto l_Fence = reinterpret_cast<DX12Fence*>(DX12RPDC->m_Fences[i]);

		// Create a fence for GPU synchronization.
		auto l_HResult = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&l_Fence->m_Fence));
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create Fence!");
			return false;
		}
#ifdef _DEBUG
		SetObjectName(DX12RPDC, l_Fence->m_Fence, ("Fence_" + std::to_string(i)).c_str());
#endif // _DEBUG

		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Fence has been created.");

		// Create an event object for the fence.
		l_Fence->m_FenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_Fence->m_FenceEvent == NULL)
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Can't create fence event!");
			return false;
		}

		InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", DX12RPDC->m_ComponentName.c_str(), " Fence event has been created.");
	}

	return true;
}

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

D3D12_BLEND DX12Helper::GetBlendFactor(BlendFactor blendFactor)
{
	D3D12_BLEND l_result;

	switch (blendFactor)
	{
	case BlendFactor::Zero: l_result = D3D12_BLEND_ZERO;
		break;
	case BlendFactor::One: l_result = D3D12_BLEND_ONE;
		break;
	case BlendFactor::SrcColor: l_result = D3D12_BLEND_SRC_COLOR;
		break;
	case BlendFactor::OneMinusSrcColor: l_result = D3D12_BLEND_INV_SRC_COLOR;
		break;
	case BlendFactor::SrcAlpha: l_result = D3D12_BLEND_SRC_ALPHA;
		break;
	case BlendFactor::OneMinusSrcAlpha: l_result = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	case BlendFactor::DestColor: l_result = D3D12_BLEND_DEST_COLOR;
		break;
	case BlendFactor::OneMinusDestColor: l_result = D3D12_BLEND_INV_DEST_COLOR;
		break;
	case BlendFactor::DestAlpha: l_result = D3D12_BLEND_DEST_ALPHA;
		break;
	case BlendFactor::OneMinusDestAlpha: l_result = D3D12_BLEND_INV_DEST_ALPHA;
		break;
	case BlendFactor::Src1Color: l_result = D3D12_BLEND_SRC1_COLOR;
		break;
	case BlendFactor::OneMinusSrc1Color: l_result = D3D12_BLEND_INV_SRC1_COLOR;
		break;
	case BlendFactor::Src1Alpha: l_result = D3D12_BLEND_SRC1_ALPHA;
		break;
	case BlendFactor::OneMinusSrc1Alpha: l_result = D3D12_BLEND_INV_SRC1_ALPHA;
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_BLEND_OP DX12Helper::GetBlendOperation(BlendOperation blendOperation)
{
	D3D12_BLEND_OP l_result;

	switch (blendOperation)
	{
	case BlendOperation::Add: l_result = D3D12_BLEND_OP_ADD;
		break;
	case BlendOperation::Substruct: l_result = D3D12_BLEND_OP_SUBTRACT;
		break;
	default:
		break;
	}

	return l_result;
}

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

bool DX12Helper::GenerateDepthStencilStateDesc(DepthStencilDesc DSDesc, DX12PipelineStateObject * PSO)
{
	PSO->m_DepthStencilDesc.DepthEnable = DSDesc.m_UseDepthBuffer;

	PSO->m_DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK(DSDesc.m_AllowDepthWrite);
	PSO->m_DepthStencilDesc.DepthFunc = GetComparisionFunction(DSDesc.m_DepthComparisionFunction);

	PSO->m_DepthStencilDesc.StencilEnable = DSDesc.m_UseStencilBuffer;

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

	return true;
}

bool DX12Helper::GenerateBlendStateDesc(BlendDesc blendDesc, DX12PipelineStateObject * PSO)
{
	// @TODO: Separate alpha and RGB blend operation
	for (size_t i = 0; i < 8; i++)
	{
		PSO->m_BlendDesc.RenderTarget[i].BlendEnable = blendDesc.m_UseBlend;
		PSO->m_BlendDesc.RenderTarget[i].SrcBlend = GetBlendFactor(blendDesc.m_SourceRGBFactor);
		PSO->m_BlendDesc.RenderTarget[i].DestBlend = GetBlendFactor(blendDesc.m_DestinationRGBFactor);
		PSO->m_BlendDesc.RenderTarget[i].BlendOp = GetBlendOperation(blendDesc.m_BlendOperation);
		PSO->m_BlendDesc.RenderTarget[i].SrcBlendAlpha = GetBlendFactor(blendDesc.m_SourceAlphaFactor);
		PSO->m_BlendDesc.RenderTarget[i].DestBlendAlpha = GetBlendFactor(blendDesc.m_DestinationAlphaFactor);
		PSO->m_BlendDesc.RenderTarget[i].BlendOpAlpha = GetBlendOperation(blendDesc.m_BlendOperation);
		PSO->m_BlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	return true;
}

bool DX12Helper::GenerateRasterizerStateDesc(RasterizerDesc rasterizerDesc, DX12PipelineStateObject * PSO)
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
	PSO->m_RasterizerDesc.DepthClipEnable = false;
	PSO->m_RasterizerDesc.MultisampleEnable = rasterizerDesc.m_AllowMultisample;
	PSO->m_RasterizerDesc.AntialiasedLineEnable = false;

	PSO->m_PrimitiveTopology = GetPrimitiveTopology(rasterizerDesc.m_PrimitiveTopology);
	PSO->m_PrimitiveTopologyType = GetPrimitiveTopologyType(rasterizerDesc.m_PrimitiveTopology);

	return true;
}

bool DX12Helper::GenerateViewportStateDesc(ViewportDesc viewportDesc, DX12PipelineStateObject * PSO)
{
	PSO->m_Viewport.Width = viewportDesc.m_Width;
	PSO->m_Viewport.Height = viewportDesc.m_Height;
	PSO->m_Viewport.MinDepth = viewportDesc.m_MinDepth;
	PSO->m_Viewport.MaxDepth = viewportDesc.m_MaxDepth;
	PSO->m_Viewport.TopLeftX = viewportDesc.m_OriginX;
	PSO->m_Viewport.TopLeftY = viewportDesc.m_OriginY;

	// Setup the scissor rect.
	PSO->m_Scissor.left = 0;
	PSO->m_Scissor.top = 0;
	PSO->m_Scissor.right = (uint64_t)PSO->m_Viewport.Width;
	PSO->m_Scissor.bottom = (uint64_t)PSO->m_Viewport.Height;

	return true;
}

bool DX12Helper::LoadShaderFile(ID3D10Blob** rhs, ShaderStage shaderStage, const ShaderFilePath & shaderFilePath)
{
	const char* l_shaderTypeName;

	switch (shaderStage)
	{
	case ShaderStage::Vertex: l_shaderTypeName = "vs_5_0";
		break;
	case ShaderStage::Hull: l_shaderTypeName = "hs_5_0";
		break;
	case ShaderStage::Domain: l_shaderTypeName = "ds_5_0";
		break;
	case ShaderStage::Geometry: l_shaderTypeName = "gs_5_0";
		break;
	case ShaderStage::Pixel: l_shaderTypeName = "ps_5_0";
		break;
	case ShaderStage::Compute: l_shaderTypeName = "cs_5_0";
		break;
	default:
		break;
	}

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT l_compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT l_compileFlags = 0;
#endif

	ID3D10Blob* l_errorMessage = 0;
	auto l_workingDir = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_workingDirW = std::wstring(l_workingDir.begin(), l_workingDir.end());
	auto l_shadeFilePathW = std::wstring(shaderFilePath.begin(), shaderFilePath.end());
	auto l_HResult = D3DCompileFromFile((l_workingDirW + m_shaderRelativePath + l_shadeFilePathW).c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", l_shaderTypeName, l_compileFlags, 0,
		rhs, &l_errorMessage);

	if (FAILED(l_HResult))
	{
		if (l_errorMessage)
		{
			auto l_errorMessagePtr = (char*)(l_errorMessage->GetBufferPointer());
			auto bufferSize = l_errorMessage->GetBufferSize();
			std::vector<char> l_errorMessageVector(bufferSize);
			std::memcpy(l_errorMessageVector.data(), l_errorMessagePtr, bufferSize);

			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: ", shaderFilePath.c_str(), " compile error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't find ", shaderFilePath.c_str(), "!");
		}
		return false;
	}

	if (l_errorMessage)
	{
		l_errorMessage->Release();
	}

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: ", shaderFilePath.c_str(), " has been compiled.");
	return true;
}