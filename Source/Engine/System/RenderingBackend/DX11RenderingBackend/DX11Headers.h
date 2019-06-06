#pragma once
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <d3dcommon.h>

struct DX11ConstantBuffer
{
	D3D11_BUFFER_DESC m_ConstantBufferDesc = D3D11_BUFFER_DESC();
	ID3D11Buffer* m_ConstantBufferPtr = 0;
	void* mappedMemory = 0;
	size_t elementCount = 0;
	size_t elementSize = 0;
};

struct DX11StructuredBuffer
{
	D3D11_BUFFER_DESC m_StructuredBufferDesc = D3D11_BUFFER_DESC();
	unsigned int elementCount = 0;
	ID3D11Buffer* m_StructuredBufferPtr = 0;
	ID3D11ShaderResourceView* SRV = 0;
	ID3D11UnorderedAccessView* UAV = 0;
};