#pragma once
#include "../common/InnoType.h"
#include "DXRenderPassComponent.h"
#include "DXShaderProgramComponent.h"

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

	DXShaderProgramComponent* m_opaquePass_DXSPC;

	DXRenderPassComponent* m_opaquePass_DXRPC;

	ShaderFilePaths m_opaquePass_shaderFilePaths = { "DX11//opaquePassCookTorranceVertex.sf" , "", "DX11//opaquePassCookTorrancePixel.sf" };

private:
	DXGeometryRenderPassComponent() {};
};
