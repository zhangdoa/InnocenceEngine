#include "DX12Helper.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "DX12RenderingServer.h"

#include "../../Engine.h"

using namespace Inno;
;

namespace Inno
{
	namespace DX12Helper
	{
		UINT GetMipLevels(TextureDesc textureDesc);

#ifdef USE_DXIL
		const char* m_shaderRelativePath = "..//Res//Shaders//DXIL//";
#else
		const wchar_t* m_shaderRelativePath = L"..//Res//Shaders//HLSL//";
#endif
	}
}

ComPtr<ID3D12CommandQueue> DX12Helper::CreateCommandQueue(D3D12_COMMAND_QUEUE_DESC * commandQueueDesc, ComPtr<ID3D12Device> device, const wchar_t *name)
{
	ComPtr<ID3D12CommandQueue> l_commandQueue;

	auto l_HResult = device->CreateCommandQueue(commandQueueDesc, IID_PPV_ARGS(&l_commandQueue));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create CommandQueue!");
		return nullptr;
	}

#ifdef INNO_DEBUG
	l_commandQueue->SetName(name);
#endif // INNO_DEBUG

	Log(Verbose, "CommandQueue has been created.");

	return l_commandQueue;
}
 
ComPtr<ID3D12CommandAllocator> DX12Helper::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12Device> device, const wchar_t *name)
{
	ComPtr<ID3D12CommandAllocator> l_commandAllocator;

	auto l_HResult = device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&l_commandAllocator));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create CommandAllocator!");
		return nullptr;
	}

#ifdef INNO_DEBUG
	l_commandAllocator->SetName(name);
#endif // INNO_DEBUG

	Log(Success, "CommandAllocator has been created.");

	return l_commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> DX12Helper::CreateCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandAllocator> commandAllocator, const wchar_t *name)
{
	ComPtr<ID3D12GraphicsCommandList> l_commandList;

	auto l_HResult = device->CreateCommandList(0, commandListType, commandAllocator.Get(), NULL, IID_PPV_ARGS(&l_commandList));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create CommandList!");
		return nullptr;
	}

#ifdef INNO_DEBUG
	l_commandList->SetName(name);
#endif // INNO_DEBUG

	return l_commandList;
}

ComPtr<ID3D12GraphicsCommandList> DX12Helper::OpenTemporaryCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandAllocator> commandAllocator)
{
	static uint64_t index = 0;

	return CreateCommandList(commandListType, device, commandAllocator, (L"TemporaryCommandList_" + std::to_wstring(index++)).c_str());
}

bool DX12Helper::CloseTemporaryCommandList(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> commandQueue)
{
	auto l_HResult = commandList->Close();
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't close temporary command list!");
	}

	ComPtr<ID3D12Fence1> l_temporaryCommandListFence;
	l_HResult = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&l_temporaryCommandListFence));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create fence for temporary command list!");
	}

	auto l_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (l_fenceEvent == nullptr)
	{
		Log(Error, "Can't create fence event for temporary command list!");
	}

	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	commandQueue->Signal(l_temporaryCommandListFence.Get(), 1);
	l_temporaryCommandListFence->SetEventOnCompletion(1, l_fenceEvent);
	WaitForSingleObject(l_fenceEvent, INFINITE);
	CloseHandle(l_fenceEvent);

	return true;
}

ComPtr<ID3D12Resource> DX12Helper::CreateUploadHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, ComPtr<ID3D12Device> device, const char* name)
{
	ComPtr<ID3D12Resource> l_uploadHeapBuffer;

	auto l_HResult = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&l_uploadHeapBuffer));

	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create upload heap buffer!");
		return nullptr;
	}

	return l_uploadHeapBuffer;
}

ComPtr<ID3D12Resource> DX12Helper::CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, ComPtr<ID3D12Device> device, D3D12_CLEAR_VALUE* clearValue, const char* name)
{
	ComPtr<ID3D12Resource> l_defaultHeapBuffer;

	auto l_HResult = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		clearValue,
		IID_PPV_ARGS(&l_defaultHeapBuffer));

	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create default heap buffer!");
		return false;
	}

	return l_defaultHeapBuffer;
}

ComPtr<ID3D12Resource> DX12Helper::CreateReadBackHeapBuffer(UINT64 size, ComPtr<ID3D12Device> device, const char* name)
{
	ComPtr<ID3D12Resource> l_readBackHeapBuffer;

	auto l_HResult = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&l_readBackHeapBuffer));

	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create read-back heap buffer!");
		return nullptr;
	}

	return l_readBackHeapBuffer;
}

D3D12_RESOURCE_DESC DX12Helper::GetDX12TextureDesc(TextureDesc textureDesc)
{
	D3D12_RESOURCE_DESC l_result = {};

	l_result.Width = textureDesc.Width;
	l_result.Height = textureDesc.Height;
	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.DepthOrArraySize = 1;
		break;
	case TextureSampler::Sampler2D:
		l_result.DepthOrArraySize = 1;
		break;
	case TextureSampler::Sampler3D:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler1DArray:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
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
	else if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_internalFormat = DXGI_FORMAT_R32_TYPELESS;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_internalFormat = DXGI_FORMAT_R24G8_TYPELESS;
	}
	else
	{
		if (textureDesc.PixelDataType == TexturePixelDataType::UByte)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::SByte)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::UShort)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::SShort)
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
		if (textureDesc.PixelDataType == TexturePixelDataType::UInt8)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt8)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt16)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt16)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt32)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt32)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
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
		else if (textureDesc.PixelDataType == TexturePixelDataType::Float32)
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

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case TextureSampler::Sampler2D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case TextureSampler::Sampler3D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		break;
	case TextureSampler::Sampler1DArray: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case TextureSampler::Sampler2DArray: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case TextureSampler::SamplerCubemap: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
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
	case TextureWrapMethod::Edge: l_result = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
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
	D3D12_RESOURCE_FLAGS l_result = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	if (textureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_result = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}

	return l_result;
}

uint32_t DX12Helper::GetTexturePixelDataSize(TextureDesc textureDesc)
{
	uint32_t l_singlePixelSize;

	switch (textureDesc.PixelDataType)
	{
	case TexturePixelDataType::UByte:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SByte:l_singlePixelSize = 1; break;
	case TexturePixelDataType::UShort:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SShort:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UInt8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SInt8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::UInt16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SInt16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UInt32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::SInt32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::Float16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::Float32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::Double:l_singlePixelSize = 8; break;
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
	if (textureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment
		|| textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}
	else
	{
		l_result = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		if (textureDesc.UseMipMap)
		{
			l_result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
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

UINT DX12Helper::GetMipLevels(TextureDesc textureDesc)
{
	if (textureDesc.UseMipMap)
	{
		return 4;
	}
	else
	{
		return 1;
	}
}

D3D12_SHADER_RESOURCE_VIEW_DESC DX12Helper::GetSRVDesc(TextureDesc textureDesc, D3D12_RESOURCE_DESC D3D12TextureDesc, uint32_t mostDetailedMip)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC l_result = {};
	l_result.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_result.Format = DXGI_FORMAT_R32_FLOAT;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	else
	{
		l_result.Format = D3D12TextureDesc.Format;
	}

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MostDetailedMip = mostDetailedMip;
		l_result.Texture1D.MipLevels = GetMipLevels(textureDesc);
		break;
	case TextureSampler::Sampler2D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MostDetailedMip = mostDetailedMip;
		l_result.Texture2D.MipLevels = GetMipLevels(textureDesc);
		break;
	case TextureSampler::Sampler3D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MostDetailedMip = mostDetailedMip;
		l_result.Texture3D.MipLevels = GetMipLevels(textureDesc);
		break;
	case TextureSampler::Sampler1DArray:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MostDetailedMip = mostDetailedMip;
		l_result.Texture1DArray.MipLevels = GetMipLevels(textureDesc);
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MostDetailedMip = mostDetailedMip;
		l_result.Texture2DArray.MipLevels = GetMipLevels(textureDesc);
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		l_result.TextureCube.MostDetailedMip = mostDetailedMip;
		l_result.TextureCube.MipLevels = GetMipLevels(textureDesc);
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC DX12Helper::GetUAVDesc(TextureDesc textureDesc, D3D12_RESOURCE_DESC D3D12TextureDesc, uint32_t mipSlice)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_result = {};

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = mipSlice;
		break;
	case TextureSampler::Sampler2D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = mipSlice;
		break;
	case TextureSampler::Sampler3D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MipSlice = mipSlice;
		l_result.Texture3D.WSize = textureDesc.DepthOrArraySize / (uint32_t)std::pow(2, mipSlice);
		break;
	case TextureSampler::Sampler1DArray:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = mipSlice;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = mipSlice;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = mipSlice;
		l_result.Texture2DArray.ArraySize = 6;
		Log(Verbose, "Use 2D texture array for UAV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_RENDER_TARGET_VIEW_DESC DX12Helper::GetRTVDesc(TextureDesc textureDesc)
{
	D3D12_RENDER_TARGET_VIEW_DESC l_result = {};

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
		break;
	case TextureSampler::Sampler2D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
		break;
	case TextureSampler::Sampler3D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MipSlice = 0;
		l_result.Texture3D.WSize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler1DArray:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = 0;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = 6;
		Log(Verbose, "Use 2D texture array for RTV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_DEPTH_STENCIL_VIEW_DESC DX12Helper::GetDSVDesc(TextureDesc textureDesc, bool stencilEnable)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC l_result = {};

	if (stencilEnable)
	{
		l_result.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		l_result.Format = DXGI_FORMAT_D32_FLOAT;
	}

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
		break;
	case TextureSampler::Sampler2D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
		break;
	case TextureSampler::Sampler3D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		Log(Verbose, "Use 2D texture array for DSV of 3D texture.");
		break;
	case TextureSampler::Sampler1DArray:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = 0;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = 6;
		Log(Verbose, "Use 2D texture array for DSV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

bool DX12Helper::CreateRootSignature(DX12RenderPassComponent* DX12RenderPassComp, ComPtr<ID3D12Device> device)
{
	std::vector<CD3DX12_ROOT_PARAMETER1> l_rootParameters(DX12RenderPassComp->m_ResourceBindingLayoutDescs.size());

	size_t l_rootDescriptorTableCount = 0;

	for (size_t i = 0; i < l_rootParameters.size(); i++)
	{
		auto l_resourceBinderLayoutDesc = DX12RenderPassComp->m_ResourceBindingLayoutDescs[i];

		if (l_resourceBinderLayoutDesc.m_IndirectBinding)
		{
			l_rootDescriptorTableCount++;
		}
	}

	std::vector<CD3DX12_DESCRIPTOR_RANGE1> l_rootDescriptorTables(l_rootDescriptorTableCount);

	size_t l_currentTableIndex = 0;

	for (size_t i = 0; i < l_rootParameters.size(); i++)
	{
		auto l_resourceBinderLayoutDesc = DX12RenderPassComp->m_ResourceBindingLayoutDescs[i];

		if (l_resourceBinderLayoutDesc.m_IndirectBinding)
		{
			switch (l_resourceBinderLayoutDesc.m_GPUResourceType)
			{
			case GPUResourceType::Sampler: l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, (uint32_t)l_resourceBinderLayoutDesc.m_SubresourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
				break;
			case GPUResourceType::Image:
				if (l_resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
				{
					l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (uint32_t)l_resourceBinderLayoutDesc.m_SubresourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
				}
				else
				{
					if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
					{
						Log(Warning, "Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
					}
					else
					{
						l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (uint32_t)l_resourceBinderLayoutDesc.m_SubresourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
					}
				}
				break;
			case GPUResourceType::Buffer:
				if (l_resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
				{
					if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
					{
						l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, (uint32_t)l_resourceBinderLayoutDesc.m_SubresourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
					}
					else
					{
						l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (uint32_t)l_resourceBinderLayoutDesc.m_SubresourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
					}
				}
				else
				{
					if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
					{
						Log(Warning, "Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
					}
					else
					{
						l_rootDescriptorTables[l_currentTableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (uint32_t)l_resourceBinderLayoutDesc.m_SubresourceCount, (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex);
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
			switch (l_resourceBinderLayoutDesc.m_GPUResourceType)
			{
			case GPUResourceType::Sampler: Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Sampler only could be accessed through a Descriptor table!");
				break;
			case GPUResourceType::Image: l_rootParameters[i].InitAsShaderResourceView((uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex, 0);
				break;
			case GPUResourceType::Buffer:
				if (l_resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
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
						Log(Warning, "Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
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

	ComPtr<ID3DBlob> l_signature = 0;
	ComPtr<ID3DBlob> l_error = 0;

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

			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " RootSignature serialization error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't serialize RootSignature!");
		}
		return false;
	}

	l_HResult = device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&DX12RenderPassComp->m_RootSignature));

	if (FAILED(l_HResult))
	{
		Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create RootSignature!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(DX12RenderPassComp, DX12RenderPassComp->m_RootSignature, "RootSignature");
#endif // INNO_DEBUG

	Log(Verbose, "", DX12RenderPassComp->m_InstanceName.c_str(), " RootSignature has been created.");

	return true;
}

bool DX12Helper::CreatePSO(DX12RenderPassComponent* DX12RenderPassComp, ComPtr<ID3D12Device> device)
{
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(DX12RenderPassComp->m_PipelineStateObject);
	auto l_DX12SPC = reinterpret_cast<DX12ShaderProgramComponent*>(DX12RenderPassComp->m_ShaderProgram);
	
	if (DX12RenderPassComp->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		GenerateDepthStencilStateDesc(DX12RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, l_PSO);
		GenerateBlendStateDesc(DX12RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, l_PSO);
		GenerateRasterizerStateDesc(DX12RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, l_PSO);
		GenerateViewportStateDesc(DX12RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, l_PSO);

		l_PSO->m_GraphicsPSODesc.pRootSignature = DX12RenderPassComp->m_RootSignature.Get();

		D3D12_INPUT_ELEMENT_DESC l_polygonLayout[6];
		uint32_t l_numElements;

		// Create the vertex input layout description.
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

		// Get a count of the elements in the layout.
		l_numElements = sizeof(l_polygonLayout) / sizeof(l_polygonLayout[0]);
		l_PSO->m_GraphicsPSODesc.InputLayout = { l_polygonLayout, l_numElements };

		// A compute shader could only be bound to a Compute pipeline
#ifdef USE_DXIL
		if (l_DX12SPC->m_CSBuffer.size())
		{
#else
		if (l_DX12SPC->m_CSBuffer)
		{
#endif
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " GPUEngineType can't be Graphics if there is a Compute shader attached!");
			return false;
		}
#ifdef USE_DXIL
		if (l_DX12SPC->m_VSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_VSBytecode;
			l_VSBytecode.pShaderBytecode = &l_DX12SPC->m_VSBuffer[0];
			l_VSBytecode.BytecodeLength = l_DX12SPC->m_VSBuffer.size();
			l_PSO->m_GraphicsPSODesc.VS = l_VSBytecode;
		}
		if (l_DX12SPC->m_HSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_HSBytecode;
			l_HSBytecode.pShaderBytecode = &l_DX12SPC->m_HSBuffer[0];
			l_HSBytecode.BytecodeLength = l_DX12SPC->m_HSBuffer.size();
			l_PSO->m_GraphicsPSODesc.HS = l_HSBytecode;
		}
		if (l_DX12SPC->m_DSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_DSBytecode;
			l_DSBytecode.pShaderBytecode = &l_DX12SPC->m_DSBuffer[0];
			l_DSBytecode.BytecodeLength = l_DX12SPC->m_DSBuffer.size();
			l_PSO->m_GraphicsPSODesc.DS = l_DSBytecode;
		}
		if (l_DX12SPC->m_GSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_GSBytecode;
			l_GSBytecode.pShaderBytecode = &l_DX12SPC->m_GSBuffer[0];
			l_GSBytecode.BytecodeLength = l_DX12SPC->m_GSBuffer.size();
			l_PSO->m_GraphicsPSODesc.GS = l_GSBytecode;
		}
		if (l_DX12SPC->m_PSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_PSBytecode;
			l_PSBytecode.pShaderBytecode = &l_DX12SPC->m_PSBuffer[0];
			l_PSBytecode.BytecodeLength = l_DX12SPC->m_PSBuffer.size();
			l_PSO->m_GraphicsPSODesc.PS = l_PSBytecode;
		}
#else
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
#endif
		if (DX12RenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
		{
			l_PSO->m_GraphicsPSODesc.NumRenderTargets = (uint32_t)DX12RenderPassComp->m_RenderPassDesc.m_RenderTargetCount;
			for (size_t i = 0; i < DX12RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				l_PSO->m_GraphicsPSODesc.RTVFormats[i] = DX12RenderPassComp->m_RTVDesc.Format;
			}
		}

		l_PSO->m_GraphicsPSODesc.DSVFormat = DX12RenderPassComp->m_DSVDesc.Format;
		l_PSO->m_GraphicsPSODesc.DepthStencilState = l_PSO->m_DepthStencilDesc;
		l_PSO->m_GraphicsPSODesc.RasterizerState = l_PSO->m_RasterizerDesc;
		l_PSO->m_GraphicsPSODesc.BlendState = l_PSO->m_BlendDesc;
		l_PSO->m_GraphicsPSODesc.SampleMask = UINT_MAX;
		l_PSO->m_GraphicsPSODesc.PrimitiveTopologyType = l_PSO->m_PrimitiveTopologyType;
		l_PSO->m_GraphicsPSODesc.SampleDesc.Count = 1;

		auto l_HResult = device->CreateGraphicsPipelineState(&l_PSO->m_GraphicsPSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create Graphics PSO!");
			return false;
		}
	}
	else
	{
		l_PSO->m_ComputePSODesc.pRootSignature = DX12RenderPassComp->m_RootSignature.Get();
#ifdef USE_DXIL
		if (l_DX12SPC->m_CSBuffer.size())
		{
			D3D12_SHADER_BYTECODE l_CSBytecode;
			l_CSBytecode.pShaderBytecode = &l_DX12SPC->m_CSBuffer[0];
			l_CSBytecode.BytecodeLength = l_DX12SPC->m_CSBuffer.size();
			l_PSO->m_ComputePSODesc.CS = l_CSBytecode;
		}
#else
		if (l_DX12SPC->m_CSBuffer)
		{
			D3D12_SHADER_BYTECODE l_CSBytecode;
			l_CSBytecode.pShaderBytecode = l_DX12SPC->m_CSBuffer->GetBufferPointer();
			l_CSBytecode.BytecodeLength = l_DX12SPC->m_CSBuffer->GetBufferSize();
			l_PSO->m_ComputePSODesc.CS = l_CSBytecode;
		}
#endif
		auto l_HResult = device->CreateComputePipelineState(&l_PSO->m_ComputePSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create Compute PSO!");
			return false;
		}
	}

#ifdef INNO_DEBUG
	SetObjectName(DX12RenderPassComp, l_PSO->m_PSO, "PSO");
#endif // INNO_DEBUG

	Log(Verbose, "", DX12RenderPassComp->m_InstanceName.c_str(), " PSO has been created.");

	return true;
}

bool DX12Helper::CreateFenceEvents(DX12RenderPassComponent *DX12RenderPassComp)
{
	bool result = true;
	for (size_t i = 0; i < DX12RenderPassComp->m_Semaphores.size(); i++)
	{
		auto l_semaphore = reinterpret_cast<DX12Semaphore*>(DX12RenderPassComp->m_Semaphores[i]);
		l_semaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_DirectCommandQueueFenceEvent == NULL)
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(),  " Can't create fence event for direct CommandQueue!");
			result = false;
		}

		l_semaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_ComputeCommandQueueFenceEvent == NULL)
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create fence event for compute CommandQueue!");
			result = false;
		}
	}

	if(result)
	{
		Log(Verbose, "Fence events have been created.");
	}

	return result;
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

bool DX12Helper::GenerateDepthStencilStateDesc(DepthStencilDesc DSDesc, DX12PipelineStateObject* PSO)
{
	PSO->m_DepthStencilDesc.DepthEnable = DSDesc.m_DepthEnable;

	PSO->m_DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK(DSDesc.m_AllowDepthWrite);
	PSO->m_DepthStencilDesc.DepthFunc = GetComparisionFunction(DSDesc.m_DepthComparisionFunction);

	PSO->m_DepthStencilDesc.StencilEnable = DSDesc.m_StencilEnable;

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

	PSO->m_RasterizerDesc.DepthClipEnable = DSDesc.m_AllowDepthClamp;

	return true;
}

bool DX12Helper::GenerateBlendStateDesc(BlendDesc blendDesc, DX12PipelineStateObject* PSO)
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

bool DX12Helper::GenerateViewportStateDesc(ViewportDesc viewportDesc, DX12PipelineStateObject* PSO)
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
#ifdef USE_DXIL
bool DX12Helper::LoadShaderFile(std::vector<char> &rhs, const ShaderFilePath &shaderFilePath)
{
	auto l_path = std::string(m_shaderRelativePath) + shaderFilePath.c_str() + ".dxil";
	rhs = g_Engine->Get<IOService>()->loadFile(l_path.c_str(), IOMode::Binary);
	return true;
}
#else
bool DX12Helper::LoadShaderFile(ID3D10Blob** rhs, ShaderStage shaderStage, const ShaderFilePath& shaderFilePath)
{
	const char* l_shaderTypeName;

	switch (shaderStage)
	{
	case ShaderStage::Vertex: l_shaderTypeName = "vs_6_0";
		break;
	case ShaderStage::Hull: l_shaderTypeName = "hs_6_0";
		break;
	case ShaderStage::Domain: l_shaderTypeName = "ds_6_0";
		break;
	case ShaderStage::Geometry: l_shaderTypeName = "gs_6_0";
		break;
	case ShaderStage::Pixel: l_shaderTypeName = "ps_6_0";
		break;
	case ShaderStage::Compute: l_shaderTypeName = "cs_6_0";
		break;
	default:
		break;
	}

#if defined(INNO_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT l_compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT l_compileFlags = 0;
#endif

	ComPtr<ID3D10Blob> l_errorMessage = 0;
	auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
	auto l_workingDirW = std::wstring(l_workingDir.begin(), l_workingDir.end());
	auto l_shadeFilePathW = std::wstring(shaderFilePath.begin(), shaderFilePath.end());
	auto l_HResult = D3DCompileFromFile((l_workingDirW + m_shaderRelativePath + l_shadeFilePathW).c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", l_shaderTypeName, l_compileFlags, 0, rhs, &l_errorMessage);

	if (FAILED(l_HResult))
	{
		if (l_errorMessage)
		{
			auto l_errorMessagePtr = (char*)(l_errorMessage->GetBufferPointer());
			auto bufferSize = l_errorMessage->GetBufferSize();
			std::vector<char> l_errorMessageVector(bufferSize);
			std::memcpy(l_errorMessageVector.data(), l_errorMessagePtr, bufferSize);

			Log(Error, "", shaderFilePath.c_str(), " compile error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			Log(Error, "Can't find ", shaderFilePath.c_str(), "!");
		}
		return false;
	}

	Log(Verbose, "", shaderFilePath.c_str(), " has been compiled.");
	return true;
}
#endif