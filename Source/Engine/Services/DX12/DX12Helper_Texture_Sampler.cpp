#include "DX12Helper_Texture.h"

using namespace Inno;

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
