#pragma once
#include "../common/InnoType.h"
#include "../system/DXHeaders.h"


class DXFinalRenderPassSingletonComponent
{
public:
	~DXFinalRenderPassSingletonComponent() {};
	
	static DXFinalRenderPassSingletonComponent& getInstance()
	{
		static DXFinalRenderPassSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;
	D3D11_BUFFER_DESC m_matrixBufferDesc;
	ID3D11Buffer* m_matrixBuffer;
	D3D11_SAMPLER_DESC m_samplerDesc;
	ID3D11SamplerState* m_sampleState;

private:
	DXFinalRenderPassSingletonComponent() {};
};
