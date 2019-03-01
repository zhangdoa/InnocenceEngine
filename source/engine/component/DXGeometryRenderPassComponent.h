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

	DXRenderPassComponent* m_opaquePass_DXRPC;

	DXShaderProgramComponent* m_opaquePass_DXSPC;

	ShaderFilePaths m_opaquePass_shaderFilePaths = { "DX11//opaquePassCookTorranceVertex.sf" , "", "DX11//opaquePassCookTorrancePixel.sf" };

private:
	DXGeometryRenderPassComponent() {};
};
