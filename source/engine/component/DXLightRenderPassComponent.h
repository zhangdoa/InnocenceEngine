#pragma once
#include "../common/InnoType.h"
#include "DXRenderPassComponent.h"
#include "DXShaderProgramComponent.h"

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

	ShaderFilePaths m_lightPass_shaderFilePaths = { "DX11//lightPassCookTorranceVertex.sf" , "", "DX11//lightPassCookTorrancePixel.sf" };

	DXShaderProgramComponent* m_DXSPC;

	DXRenderPassComponent* m_DXRPC;

private:
	DXLightRenderPassComponent() {};
};
