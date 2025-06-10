#pragma once
#include "../../Common/LogService.h"
#include "DX12Headers.h"

#include "../../Component/TextureComponent.h"
#include "../IRenderingServer.h"

namespace Inno
{
    namespace DX12Helper
    {
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
    }
}