#pragma once
#include "../common/InnoType.h"
#include "../system/DXHeaders.h"


class DXFinalRenderPassComponent
{
public:
	~DXFinalRenderPassComponent() {};
	
	static DXFinalRenderPassComponent& get()
	{
		static DXFinalRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;

	D3D11_SAMPLER_DESC m_samplerDesc;
	ID3D11SamplerState* m_samplerState;

private:
	DXFinalRenderPassComponent() {};
};
