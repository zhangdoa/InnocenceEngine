#pragma once
#include "../common/InnoType.h"
#include "DX11RenderPassComponent.h"
#include "DX11ShaderProgramComponent.h"

class DXLightRenderPassComponent
{
public:
	~DXLightRenderPassComponent() {};

	static DXLightRenderPassComponent& get()
	{
		static DXLightRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	DX11RenderPassComponent* m_DXRPC;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_lightPass_shaderFilePaths = { "DX11//lightPassCookTorranceVertex.sf" , "", "DX11//lightPassCookTorrancePixel.sf" };

	DX11CBuffer  m_lightPass_PSCBuffer;

private:
	DXLightRenderPassComponent() {};
};
