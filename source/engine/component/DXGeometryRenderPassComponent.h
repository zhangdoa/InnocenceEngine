#pragma once
#include "../common/InnoType.h"
#include "DX11RenderPassComponent.h"
#include "DX11ShaderProgramComponent.h"

class DXGeometryRenderPassComponent
{
public:
	~DXGeometryRenderPassComponent() {};

	static DXGeometryRenderPassComponent& get()
	{
		static DXGeometryRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	DX11RenderPassComponent* m_opaquePass_DXRPC;

	DX11ShaderProgramComponent* m_opaquePass_DXSPC;

	ShaderFilePaths m_opaquePass_shaderFilePaths = { "DX11//opaquePassCookTorranceVertex.sf" , "", "DX11//opaquePassCookTorrancePixel.sf" };

private:
	DXGeometryRenderPassComponent() {};
};
