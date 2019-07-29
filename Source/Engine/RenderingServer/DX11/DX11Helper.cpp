#include "DX11Helper.h"
#include "../../Core/InnoLogger.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

auto m_shaderRelativePath = L"Res//Shaders//DX11//";

D3D11_TEXTURE_DESC DX11Helper::GetDX11TextureDataDesc(TextureDataDesc textureDataDesc)
{
	D3D11_TEXTURE_DESC l_result = {};

	l_result.Width = textureDataDesc.width;
	l_result.Height = textureDataDesc.height;
	l_result.Depth = textureDataDesc.depth;
	l_result.ArraySize = 1;
	l_result.MipLevels = GetTextureMipLevels(textureDataDesc);
	l_result.Format = GetTextureFormat(textureDataDesc);
	l_result.SampleDesc.Count = 1;
	l_result.Usage = D3D11_USAGE_DEFAULT;
	l_result.BindFlags = GetTextureBindFlags(textureDataDesc);
	l_result.CPUAccessFlags = 0;

	if (textureDataDesc.magFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		l_result.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	else
	{
		l_result.MiscFlags = 0;
	}

	return l_result;
}

DXGI_FORMAT DX11Helper::GetTextureFormat(TextureDataDesc textureDataDesc)
{
	DXGI_FORMAT l_internalFormat = DXGI_FORMAT_UNKNOWN;

	if (textureDataDesc.usageType == TextureUsageType::ALBEDO)
	{
		l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_internalFormat = DXGI_FORMAT_R32_TYPELESS;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_internalFormat = DXGI_FORMAT_R24G8_TYPELESS;
	}
	else
	{
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			default: break;
			}
		}
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			default: break;
			}
		}
	}

	return l_internalFormat;
}

unsigned int DX11Helper::GetTextureMipLevels(TextureDataDesc textureDataDesc)
{
	unsigned int textureMipLevels = 1;
	if (textureDataDesc.magFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		textureMipLevels = 0;
	}

	return textureMipLevels;
}

unsigned int DX11Helper::GetTextureBindFlags(TextureDataDesc textureDataDesc)
{
	unsigned int textureBindFlags = 0;
	if (textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	}
	else if (textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	}
	else
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE;
	}

	return textureBindFlags;
}

D3D11_TEXTURE1D_DESC DX11Helper::Get1DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc)
{
	D3D11_TEXTURE1D_DESC l_result = {};

	l_result.Width = textureDataDesc.Width;
	l_result.MipLevels = textureDataDesc.MipLevels;
	l_result.ArraySize = textureDataDesc.ArraySize;
	l_result.Format = textureDataDesc.Format;
	l_result.Usage = textureDataDesc.Usage;
	l_result.BindFlags = textureDataDesc.BindFlags;
	l_result.CPUAccessFlags = textureDataDesc.CPUAccessFlags;
	l_result.MiscFlags = textureDataDesc.MiscFlags;

	return l_result;
}

D3D11_TEXTURE2D_DESC DX11Helper::Get2DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc)
{
	D3D11_TEXTURE2D_DESC l_result = {};

	l_result.Width = textureDataDesc.Width;
	l_result.Height = textureDataDesc.Height;
	l_result.MipLevels = textureDataDesc.MipLevels;
	l_result.ArraySize = textureDataDesc.ArraySize;
	l_result.Format = textureDataDesc.Format;
	l_result.SampleDesc = textureDataDesc.SampleDesc;
	l_result.Usage = textureDataDesc.Usage;
	l_result.BindFlags = textureDataDesc.BindFlags;
	l_result.CPUAccessFlags = textureDataDesc.CPUAccessFlags;
	l_result.MiscFlags = textureDataDesc.MiscFlags;

	return l_result;
}

D3D11_TEXTURE3D_DESC DX11Helper::Get3DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc)
{
	D3D11_TEXTURE3D_DESC l_result = {};

	l_result.Width = textureDataDesc.Width;
	l_result.Height = textureDataDesc.Height;
	l_result.Depth = textureDataDesc.Depth;
	l_result.MipLevels = textureDataDesc.MipLevels;
	l_result.Format = textureDataDesc.Format;
	l_result.Usage = textureDataDesc.Usage;
	l_result.BindFlags = textureDataDesc.BindFlags;
	l_result.CPUAccessFlags = textureDataDesc.CPUAccessFlags;
	l_result.MiscFlags = textureDataDesc.MiscFlags;

	return l_result;
}

UINT GetMipLevels(TextureDataDesc textureDataDesc)
{
	if (textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
		|| textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
		|| textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
		|| textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

D3D11_SHADER_RESOURCE_VIEW_DESC DX11Helper::GetSRVDesc(TextureDataDesc textureDataDesc, D3D11_TEXTURE_DESC D3D11TextureDesc)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC l_result = {};

	if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_result.Format = DXGI_FORMAT_R32_FLOAT;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_result.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	else
	{
		l_result.Format = D3D11TextureDesc.Format;
	}

	if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_1D)
	{
		l_result.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MostDetailedMip = 0;
		l_result.Texture1D.MipLevels = GetMipLevels(textureDataDesc);
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_2D)
	{
		l_result.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MostDetailedMip = 0;
		l_result.Texture2D.MipLevels = GetMipLevels(textureDataDesc);
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_3D)
	{
		l_result.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MostDetailedMip = 0;
		l_result.Texture3D.MipLevels = GetMipLevels(textureDataDesc);
	}
	else
	{
		// @TODO: Cubemap support
	}

	return l_result;
}

D3D11_UNORDERED_ACCESS_VIEW_DESC DX11Helper::GetUAVDesc(TextureDataDesc textureDataDesc, D3D11_TEXTURE_DESC D3D11TextureDesc)
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC l_result = {};

	if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_1D)
	{
		l_result.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_2D)
	{
		l_result.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_3D)
	{
		l_result.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MipSlice = 0;
	}
	else
	{
		// @TODO: Cubemap support
	}

	return l_result;
}

D3D11_RENDER_TARGET_VIEW_DESC DX11Helper::GetRTVDesc(TextureDataDesc textureDataDesc)
{
	D3D11_RENDER_TARGET_VIEW_DESC l_result = {};

	if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_1D)
	{
		l_result.Format = GetTextureFormat(textureDataDesc);
		l_result.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_2D)
	{
		l_result.Format = GetTextureFormat(textureDataDesc);
		l_result.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_3D)
	{
		l_result.Format = GetTextureFormat(textureDataDesc);
		l_result.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MipSlice = 0;
	}
	else
	{
		// @TODO: Cubemap support
	}

	return l_result;
}

D3D11_DEPTH_STENCIL_VIEW_DESC DX11Helper::GetDSVDesc(TextureDataDesc textureDataDesc, DepthStencilDesc DSDesc)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC l_result = {};

	if (DSDesc.m_UseStencilBuffer)
	{
		l_result.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		l_result.Format = DXGI_FORMAT_D32_FLOAT;
	}

	if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_1D)
	{
		l_result.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_2D)
	{
		l_result.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_3D)
	{
		// Not supported
	}
	else
	{
		// @TODO: Cubemap support
	}

	return l_result;
}

D3D11_COMPARISON_FUNC getComparisionFunction(ComparisionFunction comparisionFunction)
{
	D3D11_COMPARISON_FUNC l_result;

	switch (comparisionFunction)
	{
	case ComparisionFunction::Never: l_result = D3D11_COMPARISON_NEVER;
		break;
	case ComparisionFunction::Less: l_result = D3D11_COMPARISON_LESS;
		break;
	case ComparisionFunction::Equal: l_result = D3D11_COMPARISON_EQUAL;
		break;
	case ComparisionFunction::LessEqual: l_result = D3D11_COMPARISON_LESS_EQUAL;
		break;
	case ComparisionFunction::Greater: l_result = D3D11_COMPARISON_GREATER;
		break;
	case ComparisionFunction::NotEqual: l_result = D3D11_COMPARISON_NOT_EQUAL;
		break;
	case ComparisionFunction::GreaterEqual: l_result = D3D11_COMPARISON_GREATER_EQUAL;
		break;
	case ComparisionFunction::Always: l_result = D3D11_COMPARISON_ALWAYS;
		break;
	default:
		break;
	}

	return l_result;
}

D3D11_STENCIL_OP getStencilOperation(StencilOperation stencilOperation)
{
	D3D11_STENCIL_OP l_result;

	switch (stencilOperation)
	{
	case StencilOperation::Keep: l_result = D3D11_STENCIL_OP_KEEP;
		break;
	case StencilOperation::Zero: l_result = D3D11_STENCIL_OP_ZERO;
		break;
	case StencilOperation::Replace: l_result = D3D11_STENCIL_OP_REPLACE;
		break;
	case StencilOperation::IncreaseSat: l_result = D3D11_STENCIL_OP_INCR_SAT;
		break;
	case StencilOperation::DecreaseSat: l_result = D3D11_STENCIL_OP_DECR_SAT;
		break;
	case StencilOperation::Invert: l_result = D3D11_STENCIL_OP_INVERT;
		break;
	case StencilOperation::Increase: l_result = D3D11_STENCIL_OP_INCR;
		break;
	case StencilOperation::Decrease: l_result = D3D11_STENCIL_OP_DECR;
		break;
	default:
		break;
	}

	return l_result;
}

bool DX11Helper::GenerateDepthStencilStateDesc(DepthStencilDesc DSDesc, DX11PipelineStateObject * PSO)
{
	PSO->m_DepthStencilDesc.DepthEnable = DSDesc.m_UseDepthBuffer;

	PSO->m_DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK(DSDesc.m_AllowDepthWrite);
	PSO->m_DepthStencilDesc.DepthFunc = getComparisionFunction(DSDesc.m_DepthComparisionFunction);

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

	PSO->m_DepthStencilDesc.FrontFace.StencilFailOp = getStencilOperation(DSDesc.m_FrontFaceStencilFailOperation);
	PSO->m_DepthStencilDesc.FrontFace.StencilDepthFailOp = getStencilOperation(DSDesc.m_FrontFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilDesc.FrontFace.StencilPassOp = getStencilOperation(DSDesc.m_FrontFaceStencilPassOperation);
	PSO->m_DepthStencilDesc.FrontFace.StencilFunc = getComparisionFunction(DSDesc.m_FrontFaceStencilComparisionFunction);

	PSO->m_DepthStencilDesc.BackFace.StencilFailOp = getStencilOperation(DSDesc.m_BackFaceStencilFailOperation);
	PSO->m_DepthStencilDesc.BackFace.StencilDepthFailOp = getStencilOperation(DSDesc.m_BackFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilDesc.BackFace.StencilPassOp = getStencilOperation(DSDesc.m_BackFaceStencilPassOperation);
	PSO->m_DepthStencilDesc.BackFace.StencilFunc = getComparisionFunction(DSDesc.m_BackFaceStencilComparisionFunction);

	return true;
}

D3D11_BLEND getBlendFactor(BlendFactor blendFactor)
{
	D3D11_BLEND l_result;

	switch (blendFactor)
	{
	case BlendFactor::Zero: l_result = D3D11_BLEND_ZERO;
		break;
	case BlendFactor::One: l_result = D3D11_BLEND_ONE;
		break;
	case BlendFactor::SrcColor: l_result = D3D11_BLEND_SRC_COLOR;
		break;
	case BlendFactor::OneMinusSrcColor: l_result = D3D11_BLEND_INV_SRC_COLOR;
		break;
	case BlendFactor::SrcAlpha: l_result = D3D11_BLEND_SRC_ALPHA;
		break;
	case BlendFactor::OneMinusSrcAlpha: l_result = D3D11_BLEND_INV_SRC_ALPHA;
		break;
	case BlendFactor::DestColor: l_result = D3D11_BLEND_DEST_COLOR;
		break;
	case BlendFactor::OneMinusDestColor: l_result = D3D11_BLEND_INV_DEST_COLOR;
		break;
	case BlendFactor::DestAlpha: l_result = D3D11_BLEND_DEST_ALPHA;
		break;
	case BlendFactor::OneMinusDestAlpha: l_result = D3D11_BLEND_INV_DEST_ALPHA;
		break;
	case BlendFactor::Src1Color: l_result = D3D11_BLEND_SRC1_COLOR;
		break;
	case BlendFactor::OneMinusSrc1Color: l_result = D3D11_BLEND_INV_SRC1_COLOR;
		break;
	case BlendFactor::Src1Alpha: l_result = D3D11_BLEND_SRC1_ALPHA;
		break;
	case BlendFactor::OneMinusSrc1Alpha: l_result = D3D11_BLEND_INV_SRC1_ALPHA;
		break;
	default:
		break;
	}

	return l_result;
}

D3D11_BLEND_OP getBlendOperation(BlendOperation blendOperation)
{
	D3D11_BLEND_OP l_result;

	switch (blendOperation)
	{
	case BlendOperation::Add: l_result = D3D11_BLEND_OP_ADD;
		break;
	case BlendOperation::Substruct: l_result = D3D11_BLEND_OP_SUBTRACT;
		break;
	default:
		break;
	}

	return l_result;
}

bool DX11Helper::GenerateBlendStateDesc(BlendDesc blendDesc, DX11PipelineStateObject * PSO)
{
	// @TODO: Separate alpha and RGB blend operation
	for (size_t i = 0; i < 8; i++)
	{
		PSO->m_BlendDesc.RenderTarget[i].BlendEnable = blendDesc.m_UseBlend;
		PSO->m_BlendDesc.RenderTarget[i].SrcBlend = getBlendFactor(blendDesc.m_SourceRGBFactor);
		PSO->m_BlendDesc.RenderTarget[i].DestBlend = getBlendFactor(blendDesc.m_DestinationRGBFactor);
		PSO->m_BlendDesc.RenderTarget[i].BlendOp = getBlendOperation(blendDesc.m_BlendOperation);
		PSO->m_BlendDesc.RenderTarget[i].SrcBlendAlpha = getBlendFactor(blendDesc.m_SourceAlphaFactor);
		PSO->m_BlendDesc.RenderTarget[i].DestBlendAlpha = getBlendFactor(blendDesc.m_DestinationAlphaFactor);
		PSO->m_BlendDesc.RenderTarget[i].BlendOpAlpha = getBlendOperation(blendDesc.m_BlendOperation);
		PSO->m_BlendDesc.RenderTarget[i].RenderTargetWriteMask = 0xFF;
	}

	return true;
}

D3D_PRIMITIVE_TOPOLOGY getPrimitiveTopology(PrimitiveTopology primitiveTopology)
{
	D3D_PRIMITIVE_TOPOLOGY l_result;

	switch (primitiveTopology)
	{
	case PrimitiveTopology::Point: l_result = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	case PrimitiveTopology::Line: l_result = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	case PrimitiveTopology::TriangleList: l_result = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case PrimitiveTopology::TriangleStrip: l_result = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;
	case PrimitiveTopology::Patch: l_result = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST; // @TODO: Don't treat Patch as a primitive topology type due to the API differences
		break;
	default:
		break;
	}

	return l_result;
}

D3D11_FILL_MODE getRasterizerFillMode(RasterizerFillMode rasterizerFillMode)
{
	D3D11_FILL_MODE l_result;

	switch (rasterizerFillMode)
	{
	case RasterizerFillMode::Point: // Not supported
		break;
	case RasterizerFillMode::Wireframe: l_result = D3D11_FILL_WIREFRAME;
		break;
	case RasterizerFillMode::Solid: l_result = D3D11_FILL_SOLID;
		break;
	default:
		break;
	}

	return l_result;
}

bool DX11Helper::GenerateRasterizerStateDesc(RasterizerDesc rasterizerDesc, DX11PipelineStateObject * PSO)
{
	PSO->m_PrimitiveTopology = getPrimitiveTopology(rasterizerDesc.m_PrimitiveTopology);
	PSO->m_RasterizerDesc.AntialiasedLineEnable = true;
	if (rasterizerDesc.m_UseCulling)
	{
		PSO->m_RasterizerDesc.CullMode = rasterizerDesc.m_RasterizerCullMode == RasterizerCullMode::Front ? D3D11_CULL_FRONT : D3D11_CULL_BACK;
	}
	else
	{
		PSO->m_RasterizerDesc.CullMode = D3D11_CULL_NONE;
	}
	PSO->m_RasterizerDesc.DepthBias = 0;
	PSO->m_RasterizerDesc.DepthBiasClamp = 0; // @TODO: Depth Clamp
	PSO->m_RasterizerDesc.DepthClipEnable = false;
	PSO->m_RasterizerDesc.FillMode = getRasterizerFillMode(rasterizerDesc.m_RasterizerFillMode);
	PSO->m_RasterizerDesc.FrontCounterClockwise = (rasterizerDesc.m_RasterizerFaceWinding == RasterizerFaceWinding::CCW);
	PSO->m_RasterizerDesc.MultisampleEnable = rasterizerDesc.m_AllowMultisample;
	PSO->m_RasterizerDesc.ScissorEnable = false;
	PSO->m_RasterizerDesc.SlopeScaledDepthBias = 0.0f;

	return true;
}

bool DX11Helper::GenerateViewportStateDesc(ViewportDesc viewportDesc, DX11PipelineStateObject * PSO)
{
	PSO->m_Viewport.Width = viewportDesc.m_Width;
	PSO->m_Viewport.Height = viewportDesc.m_Height;
	PSO->m_Viewport.MinDepth = viewportDesc.m_MinDepth;
	PSO->m_Viewport.MaxDepth = viewportDesc.m_MaxDepth;
	PSO->m_Viewport.TopLeftX = viewportDesc.m_OriginX;
	PSO->m_Viewport.TopLeftY = viewportDesc.m_OriginY;

	return true;
}

D3D11_FILTER GetFilterMode(TextureFilterMethod textureFilterMethod)
{
	D3D11_FILTER l_result;

	// @TODO: Completeness of the filter
	switch (textureFilterMethod)
	{
	case TextureFilterMethod::NEAREST: l_result = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		break;
	case TextureFilterMethod::LINEAR: l_result = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: l_result = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		break;
	default:
		break;
	}

	return l_result;
}

D3D11_TEXTURE_ADDRESS_MODE GetWrapMode(TextureWrapMethod textureWrapMethod)
{
	D3D11_TEXTURE_ADDRESS_MODE l_result;

	switch (textureWrapMethod)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE: l_result = D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case TextureWrapMethod::REPEAT: l_result = D3D11_TEXTURE_ADDRESS_WRAP;
		break;
	case TextureWrapMethod::CLAMP_TO_BORDER: l_result = D3D11_TEXTURE_ADDRESS_BORDER;
		break;
	default:
		break;
	}

	return l_result;
}

bool DX11Helper::GenerateSamplerStateDesc(SamplerDesc samplerDesc, DX11PipelineStateObject * PSO)
{
	PSO->m_SamplerDesc.Filter = GetFilterMode(samplerDesc.m_FilterMethod);
	PSO->m_SamplerDesc.AddressU = GetWrapMode(samplerDesc.m_WrapMethodU);
	PSO->m_SamplerDesc.AddressV = GetWrapMode(samplerDesc.m_WrapMethodV);
	PSO->m_SamplerDesc.AddressW = GetWrapMode(samplerDesc.m_WrapMethodW);
	PSO->m_SamplerDesc.MipLODBias = 0.0f;
	PSO->m_SamplerDesc.MaxAnisotropy = samplerDesc.m_MaxAnisotropy;
	PSO->m_SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	PSO->m_SamplerDesc.BorderColor[0] = samplerDesc.m_BoardColor[0];
	PSO->m_SamplerDesc.BorderColor[1] = samplerDesc.m_BoardColor[1];
	PSO->m_SamplerDesc.BorderColor[2] = samplerDesc.m_BoardColor[2];
	PSO->m_SamplerDesc.BorderColor[3] = samplerDesc.m_BoardColor[3];
	PSO->m_SamplerDesc.MinLOD = samplerDesc.m_MinLOD;
	PSO->m_SamplerDesc.MaxLOD = samplerDesc.m_MaxLOD;

	return true;
}

bool DX11Helper::LoadShaderFile(ID3D10Blob** rhs, ShaderType shaderType, const ShaderFilePath & shaderFilePath)
{
	const char* l_shaderTypeName;

	switch (shaderType)
	{
	case ShaderType::VERTEX: l_shaderTypeName = "vs_5_0";
		break;
	case ShaderType::TCS: l_shaderTypeName = "hs_5_0";
		break;
	case ShaderType::TES: l_shaderTypeName = "ds_5_0";
		break;
	case ShaderType::GEOMETRY: l_shaderTypeName = "gs_5_0";
		break;
	case ShaderType::FRAGMENT: l_shaderTypeName = "ps_5_0";
		break;
	case ShaderType::COMPUTE: l_shaderTypeName = "cs_5_0";
		break;
	default:
		break;
	}

	ID3D10Blob* l_errorMessage = 0;
	auto l_workingDir = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_workingDirW = std::wstring(l_workingDir.begin(), l_workingDir.end());
	auto l_shadeFilePathW = std::wstring(shaderFilePath.begin(), shaderFilePath.end());
	auto l_HResult = D3DCompileFromFile((l_workingDirW + m_shaderRelativePath + l_shadeFilePathW).c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", l_shaderTypeName, D3D10_SHADER_ENABLE_STRICTNESS, 0,
		rhs, &l_errorMessage);

	if (FAILED(l_HResult))
	{
		if (l_errorMessage)
		{
			auto l_errorMessagePtr = (char*)(l_errorMessage->GetBufferPointer());
			auto bufferSize = l_errorMessage->GetBufferSize();
			std::vector<char> l_errorMessageVector(bufferSize);
			std::memcpy(l_errorMessageVector.data(), l_errorMessagePtr, bufferSize);

			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: ", shaderFilePath.c_str(), " compile error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: can't find ", shaderFilePath.c_str(), "!");
		}
		return false;
	}

	if (l_errorMessage)
	{
		l_errorMessage->Release();
	}

	InnoLogger::Log(LogLevel::Verbose, "DX11RenderingServer: ", shaderFilePath.c_str(), " has been compiled.");
	return true;
}